#pragma once

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
}
