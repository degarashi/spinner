#pragma once
#include <tuple>
#include <type_traits>

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
}

