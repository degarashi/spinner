#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "misc.hpp"
#include <unordered_set>

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
				SPNode getParent() {
					return parent.lock();
				}
			};

			class TreeNode_t : public TreeNode<TreeNode_t> {
				public:
					int		value;
					TreeNode_t(int v): value(v) {}
			};
			using SPTreeNode = std::shared_ptr<TreeNode_t>;
		}

		class TreeNodeTest : public RandomTestInitializer {};
		TEST_F(TreeNodeTest, General) {
			auto rd = getRand();
			// TreeNodeと、子を配列で持つノードでそれぞれツリーを作成
			SPNode		rootA = std::make_shared<TestNode>(0);		// 確認用
			SPNodeV		arA;
			SPTreeNode	rootB = std::make_shared<TreeNode_t>(0);	// テスト対象
			std::vector<SPTreeNode> arB;
			auto reflAB = [&]() {
				auto refl = [](auto& ar, auto& root) {
					ar.clear();
					root->iterateDepthFirst([&ar](auto& nd, int){
						ar.emplace_back(nd.shared_from_this());
						return std::decay_t<decltype(nd)>::Iterate::StepIn;
					});
				};
				refl(arA, rootA);
				refl(arB, rootB);
			};
			// ランダムにノード操作
			constexpr int N_Manipulation = 300;
			enum class E_Manip {
				Add,
				Add2,
				Add3,
				Remove,
				Recompose,
				_Num
			};
			auto fnAddNode = [](auto&& node, auto&& toAdd){
				node->addChild(toAdd);
			};
			auto fnRemNode = [](auto&& node){
				node->getParent()->removeChild(node);
			};
			reflAB();
			// <<<sibling付きのノードを挿入する場合の親付け替え>>>
			for(int i=1 ; i<N_Manipulation ; i++) {
				for(;;) {
					// ノードが1つしか無い時は追加オンリー
					E_Manip typ = (arA.size() <= 1) ? E_Manip::Add :
														static_cast<E_Manip>(rd.getUniformRange<int>(0, static_cast<int>(E_Manip::_Num)-1));
					switch(typ) {
						// 新たなノードを追加
						case E_Manip::Add:
						case E_Manip::Add2:
						case E_Manip::Add3: {
							// 挿入箇所を配列から適当に選ぶ
							int at = rd.getUniformRange<int>(0, arA.size()-1);
							// Tree-A
							auto& node_a = arA[at];
							fnAddNode(node_a, std::make_shared<TestNode>(i));
							// Tree-B
							auto& node_b = arB[at];
							fnAddNode(node_b, std::make_shared<TreeNode_t>(i));
							std::cout << boost::format("added: %1% value=%2%\n") % at % i;
							break; }
						// ノードを削除
						case E_Manip::Remove: {
							// ルート以外を選ぶ
							int at = rd.getUniformRange<int>(1, arA.size()-1);
							// Tree-A
							auto value = arA[at]->value;
							fnRemNode(arA[at]);
							// Tree-B
							fnRemNode(arB[at]);
							std::cout << boost::format("removed: %1% value=%2%\n") % at % value;
							break; }
						// ノードを別の場所に繋ぎ変え
						case E_Manip::Recompose: {
							int from = rd.getUniformRange<int>(1, arA.size()-1),
								to = rd.getUniformRange<int>(0, arA.size()-2);
							if(to >= from)
								++to;
							// Tree-A
							auto value = arA[from]->value;
							fnRemNode(arA[from]);
							fnAddNode(arA[to], arA[from]);
							// Tree-B
							fnRemNode(arB[from]);
							fnAddNode(arB[to], arB[from]);
							std::cout << boost::format("moved: %1% to %2% value=%3%\n") % from % to % value;
							break; }
						default:
							__assume(0);
					}
					break;
				}
				//{	// print out テスト
				//	std::unordered_set<TreeNode_t*> testset;
				//	rootB->iterateDepthFirst([&testset](auto& nd, int d){
				//		using Ret = typename std::decay_t<decltype(nd)>::Iterate;
				//		while(d-- > 0)
				//			std::cout << '\t';
				//		if(testset.count(&nd) == 0) {
				//			testset.emplace(&nd);
				//			std::cout << nd.value << std::endl;
				//			return Ret::StepIn;
				//		} else {
				//			std::cout << '[' << nd.value << ']' << std::endl;
				//			return Ret::Quit;
				//		}
				//	});
				//}
				//{ // 重複ノードに関するテスト
				//	std::unordered_set<TreeNode_t*> testset;
				//	rootB->iterateDepthFirst([&testset](auto& nd, int){
				//		EXPECT_EQ(testset.count(&nd), 0);
				//		testset.emplace(&nd);
				//		return std::decay_t<decltype(nd)>::Iterate::StepIn;
				//	});
				//}
				ASSERT_EQ(arA.size(), arB.size());
				reflAB();
				// 2種類のツリーで比較
				int nN = arA.size();
				for(int i=0 ; i<nN ; i++) {
					// 深度優先で巡回した時に同じ順番でノードが取り出せる筈
					ASSERT_EQ(arA[i]->value, arB[i]->value);
				}
			}
		}
	}
}

