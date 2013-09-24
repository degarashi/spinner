#pragma once
#include <type_traits>
#include <algorithm>

namespace spn {
	//! Greatest Common Divisorの計算
	template <class T, class = typename std::enable_if<std::is_integral<T>::value>::type>
	T GCD(T a, T b) {
		if(a==0 || b==0)
			return 0;
		while(a != b) {
			if(a > b)	a -= b;
			else 		b -= a;
		}
		return a;
	}
	//! Least Common Multipleの計算
	template <class T, class = typename std::enable_if<std::is_integral<T>::value>::type>
	T LCM(T a, T b) {
		if(a==0 || b==0)
			return 0;
		return (a / GCD(a,b)) * b;
	}

	template <class T, class = typename std::enable_if<std::is_integral<T>::value>::type>
	class Frac {
		T	_num, _denom;

		public:
			struct tagIrreducible {} Irreducible;
			Frac() = default;
			Frac(const T& num, const T& denom, tagIrreducible): _num(num), _denom(denom) {
				irreducible();
			}
			Frac(const T& num, const T& denom): _num(num), _denom(denom) {}

			Frac& operator += (const Frac& f) {
				if(_denom != f._denom) {
					auto lcm = LCM(_denom, f._denom);
					_num = _num * (lcm / _denom) + f._num * (lcm / f._denom);
					_denom = lcm;
				} else
					_num += f._num;
				return *this;
			}
			Frac& operator *= (const Frac& f) {
				_num *= f._num;
				_denom *= f._denom;
				return *this;
			}
			Frac& operator /= (const Frac& f) {
				Frac f2(f);
				f2.inverse();
				return *this *= f2;
			}
			Frac& operator -= (const Frac& f) {
				return *this += -f;
			}
			Frac operator - () const {
				return Frac(-_num, _denom);
			}
			T getNumber() const {
				return _num / _denom;
			}
			template <class CMP>
			bool _cmp(const Frac& f, CMP cmp) const {
				if(_denom != f._denom) {
					auto lcm = LCM(_denom, f._denom);
					return cmp(_num * (lcm / _denom),
							f._num * (lcm / f._denom));
				}
				return cmp(_num, f._num);
			}
			bool operator < (const Frac& f) const { return _cmp(f, std::less<T>()); }
			bool operator > (const Frac& f) const { return _cmp(f, std::greater<T>()); }
			bool operator <= (const Frac& f) const { return _cmp(f, std::less_equal<T>()); }
			bool operator >= (const Frac& f) const { return _cmp(f, std::greater_equal<T>()); }
			void irreducible() {
				auto gcd = GCD(_num, _denom);
				if(gcd != 0) {
					_num /= gcd;
					_denom /= gcd;
				}
			}
			void inverse() {
				std::swap(_num, _denom);
			}
	};
	using FracI = Frac<int>;
}
