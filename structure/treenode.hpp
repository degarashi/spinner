#pragma once
#include <memory>
#include <iostream>
#include "error.hpp"
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>

namespace spn {
	//! 汎用ツリー構造
	/*! Tは必ずTreeNode<T>を継承する前提 */
	template <class T>
	class TreeNode : public std::enable_shared_from_this<T> {
		private:
			friend class boost::serialization::access;
			template <class Ar>
			void serialize(Ar& ar, const unsigned int) {
				// データの出力は上位クラスがする
				// ツリー構造はSerializerクラスが担当
			}

		// 巡回参照を避けるために親ノードはweak_ptrで格納
		public:
			using this_t = TreeNode<T>;
			using pointer = this_t*;
			using SP = std::shared_ptr<T>;
			using WP = std::weak_ptr<T>;

			SP	_spChild,
				_spSibling;
			WP	_wpParent;
			static void _PrintIndent(std::ostream& os, int n) {
				while(--n >= 0)
					os << '\t';
			}

			class Serializer {
				private:
					using SPC = std::shared_ptr<const T>;
					SP		_spRoot;

					using SPArray = std::vector<SP>;
					using SPCArray = std::vector<SPC>;
					using IDXArray = std::vector<int>;
					friend class boost::serialization::access;
					BOOST_SERIALIZATION_SPLIT_MEMBER();
					template <class Ar>
					void save(Ar& ar, const unsigned int) const {
						// 配列化して出力
						SPCArray		spArray;
						_spRoot->iterateDepthFirst([&spArray](const T& node, int){
							SPC ss = node.shared_from_this();
							spArray.push_back(ss);
							return Iterate::StepIn;
						});
						// 親ノードの、配列中でのインデックス値を出力
						int nA = spArray.size();
						IDXArray	 idxArray(nA);
						for(int i=0 ; i<nA ; i++) {
							if(auto sp = spArray[i]->getParent()) {
								auto itr = std::find_if(spArray.begin(), spArray.end(), [&sp](auto& s){
									return s == sp;
								});
								idxArray[i] = itr - spArray.begin();
							} else
								idxArray[i] = -1;
						}

						ar	& BOOST_SERIALIZATION_NVP(spArray)
							& BOOST_SERIALIZATION_NVP(idxArray);
					}
					template <class Ar>
					void load(Ar& ar, const unsigned int) {
						// 配列を読み込んでツリー構築
						SPArray			spArray;
						IDXArray		idxArray;
						ar	& BOOST_SERIALIZATION_NVP(spArray)
							& BOOST_SERIALIZATION_NVP(idxArray);

						if(!spArray.empty()) {
							int nA = spArray.size();
							for(int i=0 ; i<nA ; i++) {
								auto idx = idxArray[i];
								if(idx >= 0)
									spArray[idx]->addChild(spArray[i]);
							}
							_spRoot = spArray[0];
						} else
						    _spRoot = SP();
					}

				public:
					Serializer() = default;
					Serializer(const SP& sp): _spRoot(sp) {}
					const SP& getNode() const {
					    return _spRoot;
					}
			};

		public:
			//! iterateDepthFirstの戻り値
			enum class Iterate {
				ReturnFromChild,	//!< 内部用
				StepIn,				//!< 子を巡回
				Next,				//!< 子を巡回せず兄弟ノードへ進む
				Quit				//!< 直ちに巡回を終える
			};
			TreeNode() = default;
			TreeNode(TreeNode&& t) = default;
			//! copy-ctorに置いてはリンク情報をコピーしない
			TreeNode(const TreeNode&) {}
			// コピー禁止
			TreeNode& operator = (const TreeNode&) = delete;
			// ムーブは可
			TreeNode& operator = (TreeNode&& t) = default;

			void setParent(const SP& s) {
				setParent(WP(s));
			}
			void setParent(const WP& w) {
				AssertP(Trap, w.lock().get() != this, "self-reference detected")
				_wpParent = w;
			}
			SP getParent() const {
				return SP(_wpParent.lock());
			}
			const SP& getChild() const {
				return _spChild;
			}
			const SP& getSibling() const {
				return _spSibling;
			}
			void removeChild(const SP& target) {
				AssertP(Trap, _spChild)
				if(_spChild == target) {
					// 最初の子ノードが削除対象
					target->setParent(WP());
					_spChild = target->getSibling();
					target->_spSibling = nullptr;
				} else {
					// 兄弟ノードのどれかが削除対象
					_spChild->removeSibling(nullptr, target);
				}
			}
			void removeSibling(pointer prev, const SP& target) {
				if(target.get() == this) {
					// このノードが削除対象
					AssertP(Trap, prev)
					prev->_spSibling = _spSibling;
					setParent(WP());
					_spSibling = nullptr;
					return;
				}
				AssertP(Trap, _spSibling)
				_spSibling->removeSibling(this, target);
			}
			void addChild(const SP& s) {
				AssertP(Trap, s.get() != this, "self-reference detected")
				if(_spChild)
					_spChild->addSibling(s);
				else {
					_spChild = s;
					s->setParent(this->shared_from_this());
				}
			}
			void addSibling(const SP& s) {
				AssertP(Trap, s.get() != this, "self-reference detected")
				if(_spSibling)
					_spSibling->addSibling(s);
				else {
					_spSibling = s;
					s->setParent(_wpParent);
				}
			}
			//! 深さ優先で巡回
			template <class Callback, bool BConst=false>
			Iterate iterateDepthFirst(Callback&& cb, int depth=0) {
				using thistc = std::conditional_t<BConst, const T, T>;
				Iterate t = cb(static_cast<thistc&>(*this), depth);
				if(t == Iterate::Quit)
					return Iterate::Quit;
				if(t == Iterate::StepIn) {
					if(_spChild) {
						if(_spChild->iterateDepthFirst(std::forward<Callback>(cb), depth+1) == Iterate::Quit)
							return Iterate::Quit;
					}
				}
				if(_spSibling)
					return _spSibling->iterateDepthFirst(std::forward<Callback>(cb), depth);
				return Iterate::ReturnFromChild;
			}
			template <class Callback>
			Iterate iterateDepthFirst(Callback&& cb, int depth=0) const {
				return iterateDepthFirst<Callback, true>(std::forward<Callback>(cb), depth);
			}
			template <class Callback>
			SP find(Callback&& cb) const {
				SP ret;
				iterateDepthFirst([&cb, &ret](auto& nd, int){
					if(cb(nd)) {
						ret = nd.shared_from_this();
						return Iterate::Quit;
					}
					return Iterate::StepIn;
				});
				return std::move(ret);
			}
			SP clone(const WP& parent=WP()) const {
				SP sp = std::make_shared<T>(*this->shared_from_this());
				if(_spChild)
					sp->_spChild = _spChild->clone(sp);
				if(_spSibling)
					sp->_spSibling = _spSibling->clone(parent);
				sp->_wpParent = parent;
				return std::move(sp);
			}
			//! 主にデバッグ用
			void print(std::ostream& os, int indent) const {
				_PrintIndent(os, indent);
				// 明示的なダウンキャスト
				os << *static_cast<const T*>(this);
			}
	};
	template <class T>
	inline std::ostream& operator << (std::ostream& os, const TreeNode<T>& t) {
		auto& self = const_cast<TreeNode<T>&>(t);
		self.iterateDepthFirst([&os](auto& s, int indent){
			s.print(os, indent);
			os << std::endl;
		});
		return os;
	}
}

