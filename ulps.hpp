#pragma once

namespace spn {
	//! 絶対値(std::absがconstexprでない為)
	template <class T>
	constexpr T abs(T i0) {
		return (i0<0) ? -i0 : i0;
	}
	namespace { namespace inner {
		extern void* Enabler;
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
		auto t_ULPs(T v0, T v1) {
			auto i0 = *reinterpret_cast<I*>(&v0),
				i1 = *reinterpret_cast<I*>(&v1);
			return t_ULPs2(i0, i1);
		}
	}}

	//! floatやdouble値をintに読み替え(constexpr)
	/*! NaNやInf, -Inf, Denormalizedな値は対応しない
		実行時に読み替えたい場合はreinterpret_castを使う */
	template <class T,
			  class Int=typename inner::ULPHelper<T>::Integral_t>
	constexpr Int AsIntegral(T v0) {
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

	//! FloatのULPs
	template <class T,
			  typename std::enable_if_t<std::is_floating_point<T>::value>*& = inner::Enabler>
	auto ULPs(T v0, T v1) {
		return inner::t_ULPs(v0,v1);
	}
	//! IntのULPs (=絶対値)
	template <class T,
			  typename std::enable_if_t<!std::is_floating_point<T>::value>*& = inner::Enabler>
	auto ULPs(T v0, T v1) {
		return abs(v0 - v1);
	}
	//! ULPsの計算 (constexprバージョン)
	template <class T>
	constexpr auto ULPs_C(T v0, T v1) {
		return inner::t_ULPs2(AsIntegral(v0), AsIntegral(v1));
	}
	//! ULPs(Units in the Last Place)の計算 (実行時バージョン)
	template <class T,
			class H = inner::ULPHelper<T>,
			class I = typename H::Integral_t>
	bool CompareULPs(T v0, T v1, I maxUlps) {
		AssertP(Trap, maxUlps > 0						// 負数でない
					&& maxUlps < (I(1)<<H::FracBits));	// 仮数部の桁を超えない
		return ULPs(v0,v1) <= maxUlps;
	}
	namespace inner {}
}

