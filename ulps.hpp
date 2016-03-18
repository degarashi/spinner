#pragma once
#include <algorithm>
#include "error.hpp"
#include "common.hpp"

namespace spn {
	//! 絶対値(std::absがconstexprでない為)
	template <class T>
	constexpr T abs(T i0) {
		return (i0<0) ? -i0 : i0;
	}
	namespace { namespace inner {
		static void* Enabler;
		template <class T>
		struct ULPHelper;
		template <>
		struct ULPHelper<float> {
			using Integral_t = int32_t;
			constexpr static int ExpBits = 8,
								FracBits = 23,
								ExpZero = 127;
		};
		template <>
		struct ULPHelper<double> {
			using Integral_t = int64_t;
			constexpr static int ExpBits = 11,
								FracBits = 52,
								ExpZero = 1023;
		};
	}}

	template <class T,
			class Int = typename inner::ULPHelper<T>::Integral_t>
	Int AsIntegral(const T& v0) {
		return *reinterpret_cast<const Int*>(&v0);
	}
	namespace { namespace inner {
		//! 内部用関数
		template <class I>
		constexpr I t_ULPs2(I i0, I i1) {
			constexpr I MSB1(I(1) << (sizeof(I)*8 - 1));
			if(i0 < 0)
				i0 = MSB1 - i0;
			if(i1 < 0)
				i1 = MSB1 - i1;
			if(i0 > i1)
				return i0 - i1;
			return i1 - i0;
		}
		template <class T,
					class I = typename ULPHelper<T>::Integral_t,
					class = std::enable_if_t<
								std::is_floating_point<T>::value
								&& std::is_integral<I>::value
								&& std::is_signed<I>::value
								&& sizeof(T)==sizeof(I) >>
		auto t_ULPs(const T& v0, const T& v1) {
			auto i0 = AsIntegral(v0),
				i1 = AsIntegral(v1);
			return t_ULPs2(i0, i1);
		}
		template <class T, class I, class Cmp, class Op>
		bool CmpULPs(const T& v0, const T& v1, I thresholdUlps, Cmp cmp, Op op) {
			auto i0 = AsIntegral(v0),
				i1 = AsIntegral(v1);
			constexpr I MSB1(I(1) << (sizeof(I)*8 - 1));
			if(i0 < 0)
				i0 = MSB1 - i0;
			if(i1 < 0)
				i1 = MSB1 - i1;
			return cmp(i0, op(i1, thresholdUlps));
		}
	}}

	//! floatやdouble値をintに読み替え(constexpr)
	/*! NaNやInf, -Inf, Denormalizedな値は対応しない
		実行時に読み替えたい場合はreinterpret_castを使う */
	template <class T,
			  class Int=typename inner::ULPHelper<T>::Integral_t>
	constexpr Int AsIntegral_C(T v0) {
		using Helper = inner::ULPHelper<T>;
		if(v0 == 0)
			return 0;
		// 符号部
		Int ret = (v0 < 0) ? (Int(1) << (sizeof(Int)*8-1)) : 0;
		if(v0 < 0)
			v0 *= -1;
		// 指数部
		Int count = Helper::ExpZero;
		if(v0 >= 1) {
			while(v0 >= 2) {
				v0 *= 0.5;
				++count;
			}
		} else {
			while(v0 < 1) {
				v0 *= 2;
				--count;
			}
		}
		ret |= count << Helper::FracBits;
		// 仮数部
		Int fract = 0;
		T w = 0.5;
		v0 -= 1.0;
		for(int i=0 ; i<Helper::FracBits ; i++) {
			fract <<= 1;
			if(v0 >= w) {
				fract |= 1;
				v0 -= w;
			}
			w *= 0.5;
		}
		ret |= fract;
		return ret;
	}

	template <class T,
			  typename std::enable_if_t<std::is_floating_point<T>::value>*& = inner::Enabler>
	auto ULPs_Move(const T& f, const int m) {
		using Int = typename inner::ULPHelper<T>::Integral_t;
		auto v = *reinterpret_cast<const Int*>(&f);
		v += m;
		return *reinterpret_cast<const T*>(&v);
	}
	//! ある値 + 浮動小数点数で表現できる最小の値 を返す
	template <class T>
	auto ULPs_Increment(const T& f) {
		return ULPs_Move(f, 1);
	}
	//! ある値 - 浮動小数点数で表現できる最小の値 を返す
	template <class T>
	auto ULPs_Decrement(const T& f) {
		return ULPs_Move(f, -1);
	}
	//! FloatのULPs
	template <class T,
			  typename std::enable_if_t<std::is_floating_point<T>::value>*& = inner::Enabler>
	auto ULPs(const T& v0, const T& v1) {
		return inner::t_ULPs(v0,v1);
	}
	//! IntのULPs (=絶対値)
	template <class T,
			  typename std::enable_if_t<!std::is_floating_point<T>::value>*& = inner::Enabler>
	auto ULPs(const T& v0, const T& v1) {
		return abs(v0 - v1);
	}
	//! ULPsの計算 (constexprバージョン)
	template <class T>
	constexpr auto ULPs_C(const T& v0, const T& v1) {
		return inner::t_ULPs2(AsIntegral_C(v0), AsIntegral_C(v1));
	}
	//! ULPs(Units in the Last Place)の計算 (実行時バージョン)
	template <class T,
			class H = inner::ULPHelper<T>,
			class I = typename H::Integral_t>
	bool EqULPs(const T& v0, const T& v1, I maxUlps) {
		AssertP(Trap, maxUlps >= 0)	// 負数でない
		auto fnCmp = [maxUlps](const auto& v0, const auto& v1){ return ULPs(v0,v1) <= maxUlps; };
		return _EqFunc(v0, v1, fnCmp, decltype(GetWidthHeightT<T>())());
	}
	template <class T, class I>
	bool NeULPs(const T& v0, const T& v1, I maxUlps) {
		return !EqULPs(v0, v1, maxUlps);
	}
	// v0 <= v1 + thresholdUlps
	template <class T,
			class I = typename inner::ULPHelper<T>::Integral_t>
	bool LeULPs(const T& v0, const T& v1, I thresholdUlps) {
		return inner::CmpULPs(v0, v1, thresholdUlps, std::less_equal<>(), std::plus<>());
	}
	// v0 >= v1 - thresholdUlps
	template <class T,
			class I = typename inner::ULPHelper<T>::Integral_t>
	bool GeULPs(const T& v0, const T& v1, I thresholdUlps) {
		return inner::CmpULPs(v0, v1, thresholdUlps, std::greater_equal<>(), std::minus<>());
	}
	namespace inner {}
}

