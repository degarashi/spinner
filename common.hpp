#pragma once
#include <tuple>
#include <type_traits>
#include <cmath>
#include <limits>

namespace spn {
	//! コンパイル時数値計算 & 比較
	template <int M, int N>
	struct TValue {
		enum { add = M+N,
			sub = M-N,
			less = (M>N) ? N : M,
			great = (M>N) ? M : N,
			less_eq = (M<=N) ? 1 : 0,
			great_eq = (M>=N) ? 1 : 0,
			lesser = (M<N) ? 1 : 0,
			greater = (M>N) ? 1 : 0,
			equal = M==N ? 1 : 0
		};
	};
	//! bool -> std::true_type or std::false_type
	template <int V>
	using TFCond = typename std::conditional<V!=0, std::true_type, std::false_type>::type;
	//! 2つの定数の演算結果をstd::true_type か std::false_typeで返す
	template <int N, int M>
	struct NType {
		using t_and = TFCond<(N&M)>;
		using t_or = TFCond<(N|M)>;
		using t_xor = TFCond<(N^M)>;
		using t_nand = TFCond<((N&M) ^ 0x01)>;
		using less = TFCond<(N<M)>;
		using great = TFCond<(N>M)>;
		using equal = TFCond<N==M>;
		using not_equal = TFCond<N!=M>;
		using less_eq = TFCond<(N<=M)>;
		using great_eq = TFCond<(N>=M)>;
	};
	//! 2つのintegral_constant<bool>を論理演算
	template <class T0, class T1>
	struct TType {
		constexpr static int I0 = std::is_same<T0, std::true_type>::value,
							I1 = std::is_same<T1, std::true_type>::value;
		using t_and = TFCond<I0&I1>;
		using t_or = TFCond<I0|I1>;
		using t_xor = TFCond<I0^I1>;
		using t_nand = TFCond<(I0&I1) ^ 0x01>;
	};
	//! 要素カウント
	#define countof(a)	static_cast<int>(sizeof((a))/sizeof((a)[0]))
	//! 最大値を取得
	template <int... N>
	struct TMax {
		constexpr static int result = 0; };
	template <int N0, int... N>
	struct TMax<N0, N...> {
		constexpr static int result = N0; };
	template <int N0, int N1, int... N>
	struct TMax<N0, N1, N...> {
		constexpr static int result = TValue<N0, TMax<N1, N...>::result>::great; };
	//! SFINAEで関数を無効化する際に使うダミー変数
	static void* Enabler;
	//! コンパイル時定数で数値のN*10乗を計算
	template <class T, int N, typename std::enable_if<N==0>::type*& = Enabler>
	constexpr T ConstantPow10(T value=1, std::integral_constant<int,N>* =nullptr) {
		return value;
	}
	template <class T, int N, typename std::enable_if<(N<0)>::type*& = Enabler>
	constexpr T ConstantPow10(T value=1, std::integral_constant<int,N>* =nullptr) {
		return ConstantPow10(value/10, (std::integral_constant<int,N+1>*)nullptr);
	}
	template <class T, int N, typename std::enable_if<(N>0)>::type*& = Enabler>
	constexpr T ConstantPow10(T value=1.f, std::integral_constant<int,N>* =nullptr) {
		return ConstantPow10(value*10, (std::integral_constant<int,N-1>*)nullptr);
	}

	//! std::tupleのハッシュを計算
	struct TupleHash {
		template <class Tup>
		static size_t get(size_t value, const Tup& /*tup*/, std::integral_constant<int,-1>) { return value; }
		template <class Tup, int N>
		static size_t get(size_t value, const Tup& tup, std::integral_constant<int,N>) {
			const auto& t = std::get<N>(tup);
			size_t h = std::hash<typename std::decay<decltype(t)>::type>()(t);
			value = (value ^ (h<<(h&0x07))) ^ (h>>3);
			return get(value, tup, std::integral_constant<int,N-1>());
		}
		template <class... Ts>
		size_t operator()(const std::tuple<Ts...>& tup) const {
			return get(0xdeadbeef * 0xdeadbeef, tup, std::integral_constant<int,sizeof...(Ts)-1>());
		}
	};
	//! 値が近いか
	/*! \param[in] val value to check
		\param[in] vExcept target value
		\param[in] vEps value threshold */
	template <class T, class T2, typename std::enable_if<std::is_arithmetic<T>::value>::type*& = Enabler>
	bool IsNear(const T& val, const T& vExcept, T2 vEps = std::numeric_limits<T>::epsilon()) {
		return std::fabs(vExcept-val) <= vEps;
	}
	template <class T, class... Ts>
	bool IsNearT(const std::tuple<Ts...>& /*tup0*/, const std::tuple<Ts...>& /*tup1*/, const T& /*epsilon*/, std::integral_constant<int,-1>*) {
		return true;
	}
	//! std::tuple全部の要素に対してIsNearを呼ぶ
	template <class T, int N, class... Ts, typename std::enable_if<(N>=0)>::type*& = Enabler>
	bool IsNearT(const std::tuple<Ts...>& tup0, const std::tuple<Ts...>& tup1, const T& epsilon, std::integral_constant<int,N>*) {
		return IsNear(std::get<N>(tup0), std::get<N>(tup1), epsilon)
				&& IsNearT(tup0, tup1, epsilon, (std::integral_constant<int,N-1>*)nullptr);
	}
	template <class... Ts, class T>
	bool IsNear(const std::tuple<Ts...>& tup0, const std::tuple<Ts...>& tup1, const T& epsilon) {
		return IsNearT<T>(tup0, tup1, epsilon, (std::integral_constant<int,sizeof...(Ts)-1>*)nullptr);
	}

	//! 浮動少数点数の値がNaNになっているか
	template <class T, class = typename std::enable_if<std::is_floating_point<T>::value>::type>
	bool IsNaN(const T& val) {
		return !(val>=T(0)) && !(val<T(0)); }
	//! 浮動少数点数の値がNaN又は無限大になっているか
	template <class T, class = typename std::enable_if<std::is_floating_point<T>::value>::type>
	bool IsOutstanding(const T& val) {
		auto valA = std::fabs(val);
		return valA==std::numeric_limits<float>::infinity() || IsNaN(valA); }

	//! std::tupleの要素ごとの距離(IsNear)比較
	template <class T, int NPow>
	struct TupleNear {
		template <class P>
		bool operator()(const P& t0, const P& t1) const {
			return IsNear(t0, t1, spn::ConstantPow10<T,NPow>());
		}
	};
}

