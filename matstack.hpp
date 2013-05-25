#pragma once
#include "spn_math.hpp"
#include "matrix.hpp"
#include <vector>
#include <cassert>

namespace spn {
	//! 行列スタックにpushした時の積算を定義
	struct MatStackTag {
		struct PushLeft {
			template <class MT>
			static void apply(MT& mt, const MT& m) {
				mt = m * mt;
			}
		};
		struct PushRight {
			template <class MT>
			static void apply(MT& mt, const MT& m) {
				mt *= m;
			}
		};
	};
	//! 座標変換などで使用する行列スタック
	/*! PushLeft: top = m * top
		PushRight: top = top * m
		任意の位置を参照することも出来る */
	template <class MT, class PDir>
	class MatStack : MatStackTag {
		using MStack = std::vector<MT>;
		MStack	_mstk;
		MT		_curMat;
		
		public:
			MatStack() {
				clear();
			}
			void clear() {
				_mstk.resize(1);
				_curMat = _mstk[0] = MT(MT::TagIdentity);
			}
			//! 今いくつ目印があるか
			size_t size() const {
				return _mstk.size();
			}
			//! 1つ前の目印まで戻す
			void pop() {
				_mstk.pop_back();
				assert(!_mstk.empty());
				_curMat = _mstk.back();
			}
			//! 直前の目印を復元
			void resetTop() {
				_curMat = _mstk.back();
			}
			//! 目印を付ける
			void push() {
				_mstk.push_back(_curMat);
			}
			//! カレント行列を更新
			void apply(const MT& m) {
				PDir::apply(_curMat, m);
			}
			//! 行列を積む
			/*! apply(m), push() と同義 */
			void push(const MT& m) {
				PDir::apply(_curMat, m);
				_mstk.push_back(_curMat);
			}
			//! カレントの行列を取得
			const MT& top() const {
				return _curMat;
			}
			
			//! popと同時にfrontを出力
			MatStack& operator >> (MT& dst) {
				dst = top();
				pop();
				return *this;
			}
			//! pushと同義
			MatStack& operator << (const MT& m) {
				push(m);
				return *this;
			}
			//! 任意の位置の行列を取得
			/*! \param n 先頭からの距離 */
			const MT& getAt(int n) const {
				return _mstk[_mstk.size()-n-1];
			}
	};
}
