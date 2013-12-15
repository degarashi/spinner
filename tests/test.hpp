#pragma once
#include <random>
#include "../optional.hpp"
#include "../matrix.hpp"

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

		struct Random {
			using OPMT = spn::Optional<std::mt19937>;
			using OPDistF = spn::Optional<std::uniform_real_distribution<float>>;
			using OPDistI = spn::Optional<std::uniform_int_distribution<int>>;
			OPMT	_opMt;
			OPDistF	_opDistF;
			OPDistI _opDistI;

			Random();
			float randomF();
			int randomI();
			int randomI(int from, int to);
			int randomIPositive(int to=std::numeric_limits<int>::max());
			static Random& getInstance();
		};
	}
}
