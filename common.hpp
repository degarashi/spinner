
namespace spn {
	//! コンパイル時数値計算 & 比較
	template <int M, int N>
	struct TValue {
		enum { add = M+N,
			sub = M-N,
			less = (M>N) ? N : M,
			great = (M>N) ? M : N,
			lesser = (M>N) ? false : true,
			greater = (M>N) ? true : false};
	};
	//! bool値による型の選択
	template <int N, class T, class F>
	struct SelectType {
		using type = F;
	};
	template <class T, class F>
	struct SelectType<1,T,F> {
		using type = T;
	};
	#define countof(a)	(sizeof((a))/sizeof((a)[0]))
}
