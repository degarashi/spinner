
namespace spn {
	//! コンパイル時数値計算 & 比較
	template <int N, int M>
	struct TValue {
		enum { add = N+M,
			sub = N-M,
			less = (N>M) ? M : N,
			great = (N>M) ? N : M,
			lesser = (N>M) ? false : true,
			greater = (N>M) ? true : false};
	};
	#define countof(a)	(sizeof((a))/sizeof((a)[0]))
}
