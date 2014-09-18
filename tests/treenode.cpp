#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "structure/treenode.hpp"
#include <unordered_set>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

namespace spn {
	namespace test {
		namespace {
			struct TestNode;
			using SPNode = std::shared_ptr<TestNode>;
			using WPNode = std::weak_ptr<TestNode>;
			using SPNodeV = std::vector<SPNode>;
			struct TestNode : std::enable_shared_from_this<TestNode> {
				SPNodeV		child;
				WPNode		parent;
				int			value;

				enum class Iterate {
					ReturnFromChild,
					StepIn,
					Next,
					Quit
				};
				TestNode(int val): value(val) {}
				void addChild(const SPNode& nd) {
					ASSERT_TRUE(std::find(child.begin(), child.end(), nd) == child.end());
					child.push_back(nd);
					ASSERT_FALSE(nd->getParent());
					nd->parent = shared_from_this();
				}
				void removeChild(const SPNode& nd) {
					auto itr = std::find(child.begin(), child.end(), nd);
					ASSERT_TRUE(itr != child.end());
					child.erase(itr);
					ASSERT_EQ(nd->getParent().get(), this);
					nd->parent = WPNode();
				}
				template <class Callback>
				void iterateDepthFirst(Callback&& cb, int depth=0) {
					cb(*this, depth);
					for(auto& c : child)
						c->iterateDepthFirst(std::forward<Callback>(cb), depth+1);
				}
				SPNode getChild() const {
					if(child.empty())
						return SPNode();
					return child[0];
				}
				SPNode getParent() {
					return parent.lock();
				}
			};

			class TreeNode_t : public TreeNode<TreeNode_t> {
				using base_t = TreeNode<TreeNode_t>;
				private:
					friend class boost::serialization::access;
					template <class Ar>
					void serialize(Ar& ar, const unsigned int) {
						ar	& BOOST_SERIALIZATION_BASE_OBJECT_NVP(base_t);
					}
				public:
					int		value;
					TreeNode_t(int v): value(v) {}
			};
			using SPTreeNode = std::shared_ptr<TreeNode_t>;

			void CheckParent(const SPTreeNode& nd) {
				auto c = nd->getChild();
				while(c) {
					// 子ノードの親をチェック
					auto p = c->getParent();
					ASSERT_EQ(p.get(), nd.get());
					// 下層のノードを再帰的にチェック
					CheckParent(c);
					// 兄弟ノードをチェック
					c = c->getSibling();
				}
			}
		}
		template <class A, class B>
		void CheckEqual(const A& a, const B& b) {
			ASSERT_EQ(a.size(), b.size());
			int nN = a.size();
			for(int i=0 ; i<nN ; i++)
				ASSERT_EQ(a[i]->value, b[i]->value);
		}
		template <class T>
		class TestTree {
			using SP = std::shared_ptr<T>;
			using VEC = std::vector<SP>;
			SP		_spRoot;
			VEC		_spArray;

			void _reflArray() {
				_spArray = plain();
			}
			public:
				using Type = T;
				TestTree(int value): _spRoot(std::make_shared<T>(value)) {
					_reflArray();
				}
				//! 要素の追加
				void add(int n, const SP& node) {
					_spArray.at(n)->addChild(node);
					_reflArray();
				}
				//! 要素の削除
				SP rem(int n) {
					SP sp = _spArray.at(n);
					if(auto pr = sp->getParent()) {
						pr->removeChild(sp);
					} else {
						_spRoot = sp->getChild();
						EXPECT_NE(static_cast<T*>(nullptr), _spRoot.get());
					}
					_reflArray();
					return std::move(sp);
				}
				//! 要素の入れ替え
				SP swapAt(int from, int to) {
					auto node = rem(from);
					add(to % size(), node);
					return std::move(node);
				}

				//! 深度優先で配列展開
				VEC plain() const {
					return Plain(_spRoot);
				}
				static VEC Plain(const SP& sp) {
					VEC ret;
					sp->iterateDepthFirst([&ret](auto& nd, int){
						ret.emplace_back(nd.shared_from_this());
						return std::decay_t<decltype(nd)>::Iterate::StepIn;
					});
					return std::move(ret);
				}
				const VEC& getArray() const {
					return _spArray;
				}
				const SP& getRoot() const {
					return _spRoot;
				}
				int size() const {
					return _spArray.size();
				}
		};
		template <class CB>
		int _CallTS(CB&& cb) { return -1; }
		template <class CB, class T, class... Ts>
		int _CallTS(CB&& cb, T& tree, Ts&&... ts) {
			int res = cb(tree);
			_CallTS(std::forward<CB>(cb), std::forward<Ts>(ts)...);
			return res;
		}
		template <class RD, class T, class... Ts>
		void RandomManipulate(RD& rd, int value, T&& t, Ts&&... ts) {
			enum class E_Manip {
				Add,
				Add2,
				Add3,
				Remove,
				Recompose,
				_Num
			};
			// ノードが1つしか無い時は追加オンリー
			E_Manip typ = (t.size() <= 1) ? E_Manip::Add :
							static_cast<E_Manip>(rd.template getUniformRange<int>(0, static_cast<int>(E_Manip::_Num)-1));
			switch(typ) {
				// 新たなノードを追加
				case E_Manip::Add:
				case E_Manip::Add2:
				case E_Manip::Add3: {
					// 挿入箇所を配列から適当に選ぶ
					int at = rd.template getUniformRange<int>(0, t.size()-1);
					// Tree-A
					value = _CallTS([value, at](auto& t){
										t.add(at, std::make_shared<typename std::decay_t<decltype(t)>::Type>(value));
										return value;
									}, std::forward<T>(t), std::forward<Ts>(ts)...);
					// std::cout << boost::format("added: %1% value=%2%\n") % at % value;
					break; }
				// ノードを削除
				case E_Manip::Remove: {
					// ルート以外を選ぶ
					int at = rd.template getUniformRange<int>(1, t.size()-1);
					value = _CallTS([at](auto& t){
										return t.rem(at)->value;
									}, std::forward<T>(t), std::forward<Ts>(ts)...);
					// std::cout << boost::format("removed: %1% value=%2%\n") % at % value;
					break; }
				// ノードを別の場所に繋ぎ変え
				case E_Manip::Recompose: {
					int from = rd.template getUniformRange<int>(1, t.size()-1),
						to = rd.template getUniformRange<int>(0, t.size()-2);
					if(to >= from)
						++to;

					value = _CallTS([from, to](auto& t){
										return t.swapAt(from, to)->value;
									}, std::forward<T>(t), std::forward<Ts>(ts)...);
					// std::cout << boost::format("moved: %1% to %2% value=%3%\n") % from % to % value;
					break; }
				default:
					__assume(0);
			}
		}

		//! print out テスト
		template <class SP>
		void PrintOut(SP& root) {
			using T = typename SP::element_type;
			std::unordered_set<const T*> testset;
			root->iterateDepthFirst([&testset](auto& nd, int d){
				using Ret = typename std::decay_t<decltype(nd)>::Iterate;
				while(d-- > 0)
					std::cout << '\t';
				if(testset.count(&nd) == 0) {
					testset.emplace(&nd);
					std::cout << nd.value << std::endl;
					return Ret::StepIn;
				} else {
					std::cout << '[' << nd.value << ']' << std::endl;
					return Ret::Quit;
				}
			});
		}
		//! 重複ノードに関するテスト
		template <class TR>
		void DuplNodeTest(TR& tree)	{
			using T = typename TR::Type;
			std::unordered_set<T*> testset;
			tree.getRoot()->iterateDepthFirst([&testset](auto& nd, int){
				EXPECT_EQ(testset.count(&nd), 0);
				testset.emplace(&nd);
				return std::decay_t<decltype(nd)>::Iterate::StepIn;
			});
		}
		class TreeNodeTest : public RandomTestInitializer {};
		TEST_F(TreeNodeTest, General) {
			auto rd = getRand();
			// TreeNodeと、子を配列で持つノードでそれぞれツリーを作成
			TestTree<TestNode>		treeA(0);		// 確認用
			TestTree<TreeNode_t>	treeB(0);		// テスト対象
			// ランダムにノード操作
			constexpr int N_Manipulation = 100;
			for(int i=1 ; i<N_Manipulation ; i++)
				RandomManipulate(rd, i, treeA, treeB);

			// 親ノードの接続確認
			CheckParent(treeB.getRoot());
			// Cloneしたツリーでも確認
			CheckParent(treeB.getRoot()->clone());
			ASSERT_EQ(treeA.size(), treeB.size());
			// 2種類のツリーで比較
			// (深度優先で巡回した時に同じ順番でノードが取り出せる筈)
			CheckEqual(treeA.getArray(), treeB.getArray());
		}

		using SerializeTest = TreeNodeTest;
		TEST_F(TreeNodeTest, TreeNode) {
			auto rd = getRand();
			// ランダムなツリーを作る
			TestTree<TreeNode_t>	tree(0);
			constexpr int N_Manipulation = 100;
			for(int i=1 ; i<N_Manipulation ; i++)
				RandomManipulate(rd, i, tree);

			// シリアライズ
			TreeNode_t::Serializer sl(tree.getRoot());
			auto ar0 = TestTree<TreeNode_t>::Plain(sl.getNode());
			std::stringstream ss;
			boost::archive::xml_oarchive oa(ss, boost::archive::no_header);
			oa << boost::serialization::make_nvp("test", sl);
			std::cout << ss.str() << std::endl;

			// デシリアライズ
			boost::archive::xml_iarchive ia(ss, boost::archive::no_header);
			ia >> boost::serialization::make_nvp("test", sl);
			auto ar1 = TestTree<TreeNode_t>::Plain(sl.getNode());

			// シリアライズ前後でデータを比べる
			CheckEqual(ar0, ar1);
		}
	}
}
namespace boost {
	namespace serialization {
		template <class Ar>
		void load_construct_data(Ar& ar, spn::test::TreeNode_t* node, const unsigned int) {
			int value;
			ar & make_nvp("value", value);
			new(node) spn::test::TreeNode_t(value);
		}
		template <class Ar>
		void save_construct_data(Ar& ar, const spn::test::TreeNode_t* node, const unsigned int) {
			ar & make_nvp("value", node->value);
		}
	}
}

