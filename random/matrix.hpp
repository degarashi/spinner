#pragma once
#include "../structure/range.hpp"

namespace spn {
	namespace random {
		// --------------- matrix ---------------
		constexpr float DefaultRMatValue = 1e4f;
		// ランダム値のデフォルト範囲
		constexpr RangeF DefaultRMatRange{-DefaultRMatValue, DefaultRMatValue};
		//! 要素の値がランダムな行列
		template <class MT, class RDF>
		auto GenRMatF(const RDF& rdf) {
			MT m;
			for(int i=0 ; i<MT::height ; i++) {
				for(int j=0 ; j<MT::width ; j++)
					m.ma[i][j] = rdf();
			}
			return m;
		}
		template <class MT, class RDF>
		auto GenRMat(const RDF& rdf, const RangeF& r=DefaultRMatRange) {
			auto rc = [&](){ return rdf(r); };
			return GenRMatF<MT>(rc);
		}
	}
}
