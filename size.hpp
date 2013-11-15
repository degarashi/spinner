#pragma once
#include "bits.hpp"
#include <algorithm>

namespace spn {
	//! 任意の型の縦横サイズ
	template <class T>
	struct _Size {
		T	width, height;

		_Size() = default;
		_Size(const T& s): width(s), height(s) {}
		_Size(const T& w, const T& h): width(w), height(h) {}
		template <class T2>
		_Size(const _Size<T2>& s): width(s.width), height(s.height) {}
		_Size& operator *= (const T& s) {
			width *= s;
			height *= s;
			return *this;
		}
		template <class V>
		void shiftR(int n, typename std::enable_if<std::is_integral<V>::value>::type* = nullptr) {
			width >>= n;
			height >>= n;
		}
		template <class V>
		void shiftR(int n, typename std::enable_if<!std::is_integral<V>::value>::type* = nullptr) { *this *= std::pow(2.f, n); }
		_Size& operator >>= (int n) {
			shiftR<T>(n);
			return *this;
		}
		//! 指定したビット数分、右シフトしてもし値がゼロになったら1をセットする
		void shiftR_one(int n) {
			width >>= n;
			width |= ~Bit::ZeroOrFull(width) & 0x01;
			height >>= n;
			height |= ~Bit::ZeroOrFull(height) & 0x01;
		}
		bool operator == (const _Size& s) const {
			return width == s.width &&
					height == s.height;
		}
		bool operator != (const _Size& s) const { return !(this->operator == (s)); }
		bool operator <= (const _Size& s) const {
			return width <= s.width &&
					height <= s.height;
		}
	};
	using Size = _Size<uint32_t>;
	using SizeF = _Size<float>;

	//! 任意の型の2D矩形
	template <class T>
	struct _Rect {
		union {
			struct {
				T	x0, x1, y0, y1;
			};
			T	ar[4];
		};

		_Rect() = default;
		_Rect(const _Rect& r) = default;
		_Rect(const T& x_0, const T& x_1, const T& y_0, const T& y_1):
			x0(x_0), x1(x_1), y0(y_0), y1(y_1) {}

		T width() const { return x1-x0; }
		T height() const { return y1-y0; }
		void addOffset(const T& x, const T& y) {
			x0 += x;
			x1 += x;
			y0 += y;
			y1 += y;
		}
		bool hit(const _Rect& r) const {
			if(x1 < r.x0 || r.x1 < x0 ||
				y1 < r.y0 || r.y1 < y0)
				return false;
			return true;
		}
		void shrinkRight(const T& s) {
			x1 = std::max(x0, x1-s);
		}
		void shrinkBottom(const T& s) {
			y1 = std::max(y0, y1-s);
		}
		void shrinkLeft(const T& s) {
			x0 = std::min(x1, x0+s);
		}
		void shrinkTop(const T& s) {
			y0 = std::min(y1, y0+s);
		}
		_Size<T> size() const {
			return _Size<T>(width(), height());
		}
		template <class T2>
		_Rect& operator *= (const T2& t) {
			for(auto& a : ar)
				a *= t;
			return *this;
		}
	};
	using Rect = _Rect<int>;
	using RectF = _Rect<float>;

	//! 値の低い方に補正
	template <class T>
	struct PP_Lower {
		static T proc(const T& t) {
			T ret = spn::Bit::LowClear(t);
			return std::max(T(1), ret);
		}
	};
	//! 値の高い方に補正
	template <class T>
	struct PP_Higher {
		static T proc(const T& t) {
			T ret = spn::Bit::LowClear(t);
			if(t & ~ret)
				ret <<= 1;
			return std::max(T(1), ret);
		}
	};

	//! 2の乗数しか代入できない型
	/*! 0も許可されない */
	template <class T, template <class> class Policy>
	class PowValue {
		using P = Policy<T>;
		T _value;

		public:
			//TODO: テンプレートによる静的な値補正
			PowValue(const T& t): _value(P::proc(t)) {}
			PowValue(const PowValue& p) = default;

			PowValue& operator = (const PowValue& p) = default;
			PowValue& operator = (const T& t) {
				// 2の乗数に補正 (動的なチェック)
				_value = P::proc(t);
				return *this;
			}
			const T& get() const { return _value; }
			operator const T& () const { return _value; }
	};
	using PowInt = PowValue<uint32_t, PP_Higher>;
	using PowSize = _Size<PowInt>;
}
