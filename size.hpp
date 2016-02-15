#pragma once
#include "bits.hpp"
#include <algorithm>
#include <iostream>

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
			return *this = *this * s; }
		_Size& operator /= (const T& s) {
			return *this = *this / s; }
		_Size operator / (const T& s) const {
			return _Size(width/s, height/s); }
		_Size operator * (const T& s) const {
			return _Size(width*s, height*s); }
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
		//! 1右ビットシフトした値を2の累乗に合わせる
		void shiftR_2pow() {
			auto fn = [](const T& t) -> T {
				using UT = std::make_unsigned_t<T>;
				UT ut(t);
				auto v = Bit::LowClear(ut);
				if(v != ut)
					return v;
				return v >> 1;
			};
			width = fn(width);
			height = fn(height);
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
		//! 領域の拡大縮小
		/*!
			負数で領域の縮小
			\return		縮小によって面積が0になった場合はtrue, それ以外はfalse
		*/
		bool expand(const T& w, const T& h) {
			bool ret = false;
			width += w;
			height += h;
			if(width <= 0) {
				width = 0;
				ret = true;
			}
			if(height <= 0) {
				height = 0;
				ret = true;
			}
			return ret;
		}
		bool expand(const T& w) {
			return expand(w,w);
		}
		template <class V>
		auto toSize() const {
			return _Size<V>(width, height);
		}
	};
	template <class T>
	std::ostream& operator << (std::ostream& os, const spn::_Size<T>& s) {
		return os << "Size {width=" << s.width << ", height=" << s.height << "}";
	}
	using Size = _Size<int32_t>;
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
		#define DEF_OP(op) \
			_Rect& operator op##= (const T& t) { \
				return *this = *this op t; } \
			_Rect operator op (const T& t) const { \
				_Rect r; \
				for(int i=0 ; i<int(countof(ar)) ; i++) \
					r.ar[i] = ar[i] op t; \
				return r; \
			}
		DEF_OP(+)
		DEF_OP(-)
		DEF_OP(*)
		DEF_OP(/)
		#undef DEF_OP
		//! 領域の拡大縮小
		/*!
			負数で領域の縮小
			\return		縮小によって面積が0になった場合はtrue, それ以外はfalse
		*/
		bool expand(const T& w, const T& h) {
			bool ret = false;
			x0 -= w;
			x1 += w;
			y0 -= h;
			y1 += h;
			if(x0 >= x1) {
				auto d = x0 - x1;
				x1 += d/2;
				x0 = x1;
				ret = true;
			}
			if(y0 >= y1) {
				auto d = y0 - y1;
				y1 += d/2;
				y0 = y1;
				ret = true;
			}
			return ret;
		}
		bool expand(const T& w) {
			return expand(w,w);
		}
		template <class V>
		auto toRect() const {
			return _Rect<V>(x0, x1, y0,y1);
		}
	};
	using Rect = _Rect<int32_t>;
	using RectF = _Rect<float>;
	template <class T>
	std::ostream& operator << (std::ostream& os, const spn::_Rect<T>& r) {
		return os << "Rect {x0=" << r.x0 << ", x1=" << r.x1 <<
				", y0=" << r.y0 << ", y1=" << r.y1 << "}";
	}

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
	template <class... Ts>
	std::ostream& operator << (std::ostream& os, const spn::PowValue<Ts...>& p) {
		return os << p.get();
	}
}
