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
			greater = (M>N) ? 1 : 0};
	};
	#define countof(a)	(sizeof((a))/sizeof((a)[0]))
}
