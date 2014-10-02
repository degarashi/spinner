#pragma once
#include <memory>
#include <iostream>
#include "error.hpp"
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>

// clangでshared_ptrが定義されない問題への一時的な対処
#include <boost/serialization/singleton.hpp>
#include <boost/serialization/extended_type_info.hpp>
#undef BOOST_NO_CXX11_SMART_PTR
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>

namespace spn {
	//! 汎用ツリー構造
	/*! Tは必ずTreeNode<T>を継承する前提 */
	template <class T>
	class TreeNode : public std::enable_shared_from_this<T> {
		private:
			friend class boost::serialization::access;
			template <class Ar>
			void serialize(Ar& ar, const unsigned int) {
				ar	& BOOST_SERIALIZATION_NVP(_spChild)
					& BOOST_SERIALIZATION_NVP(_wpParent)
					& BOOST_SERIALIZATION_NVP(_spSibling);
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
				return const_cast<this_t*>(this)->iterateDepthFirst<Callback, true>(std::forward<Callback>(cb), depth);
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

