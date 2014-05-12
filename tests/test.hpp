#pragma once
#ifdef WIN32
	#include <intrin.h>
#endif
#include "../matrix.hpp"
#include "../error.hpp"

namespace spn {
	namespace test {
		template <int M, int N>
		const float* SetValue(MatT<M,N,true>& m, const float* src) {
			for(int i=0 ; i<M ; i++)
				for(int j=0 ; j<N ; j++)
					m.ma[i][j] = *src++;
			return src;
		}
		template <int M, int N>
		const float* CheckValue(MatT<M,N,true>& m, const float* src) {
			for(int i=0 ; i<M ; i++)
				for(int j=0 ; j<N ; j++)
					Assert(Trap, m.ma[i][j]==*src++)
			return src;
		}
		template <int N>
		const float* SetValue(VecT<N,true>& v, const float* src) {
			for(int i=0 ; i<N ; i++)
				v.m[i] = *src++;
			return src;
		}
		template <int N>
		const float* CheckValue(VecT<N,true>& v, const float* src) {
			for(int i=0 ; i<N ; i++)
				Assert(Trap, v.m[i]==*src++)
			return src;
		}
	}
}

