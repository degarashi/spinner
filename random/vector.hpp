#pragma once
#include "structure/angle.hpp"
#include "structure/range.hpp"

namespace spn {
	namespace random {
		// --------------- vector ---------------
		//! 要素の値がランダムなベクトル
		template <class VT, class RDF>
		auto GenRVecF(const RDF& rdf) {
			VT v;
			for(int i=0 ; i<VT::width ; i++)
				v.m[i] = rdf();
			return v;
		}
		constexpr float DefaultRVecValue = 1e4f;
		// ランダム値のデフォルト範囲
		constexpr RangeF DefaultRVecRange{-DefaultRVecValue, DefaultRVecValue};
		template <class VT, class RDF>
		auto GenRVec(const RDF& rdf, const RangeF& r=DefaultRVecRange) {
			auto rc = [&](){ return rdf(r); };
			return GenRVecF<VT>(rc);
		}

		//! 条件を満たすランダムベクトルを生成
		template <class VT, class RDF, class Chk>
		auto GenRVecCnd(const RDF& rdf, Chk&& chk, const RangeF& r=DefaultRVecRange) {
			VT v;
			do {
				v = GenRVec<VT>(rdf, r);
			} while(!std::forward<Chk>(chk)(v));
			return v;
		}
		//! ランダムなベクトル（但し長さがゼロではない）
		template <class VT, class RDF>
		auto GenRVecLen(const RDF& rdf, float th, const RangeF& r=DefaultRVecRange) {
			return GenRVecCnd<VT>(rdf, [th](auto& v){ return v.length() >= th; }, r);
		}
		//! ランダムなベクトル (但し全ての成分の絶対値がそれぞれ基準範囲内)
		template <class VT, class RDF>
		auto GenRVecAbs(const RDF& rdf, const RangeF& rTh, const RangeF& r=DefaultRVecRange) {
			return GenRVecCnd<VT>(rdf,
					[rTh](auto& v){
						for(int i=0 ; i<VT::width ; i++) {
							if(!rTh.hit(v.m[i]))
								return false;
						}
						return true;
					},
					r);
		}
		//! ランダムな方向ベクトル
		template <class VT, class RDF>
		auto GenRDir(const RDF& rdf) {
			return GenRVecLen<VT>(rdf, 1e-4f, {-1.f, 1.f}).normalization();
		}
	}
}
