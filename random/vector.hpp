#pragma once
#include "structure/angle.hpp"
#include "structure/range.hpp"

namespace spn {
	namespace random {
		// --------------- vector ---------------
		//! 要素の値がランダムなベクトル
		template <class VT, class RD>
		auto GenRVecF(RD& rd) {
			VT v;
			for(int i=0 ; i<VT::width ; i++)
				v.m[i] = rd();
			return v;
		}
		template <class VT, class RD>
		auto GenRVec(RD& rd, const RangeF& r) {
			auto rc = [&](){ return rd.template getUniform<float>(r); };
			return GenRVecF<VT>(rc);
		}
		constexpr float DefaultRVecValue = 1e4f;
		// ランダム値のデフォルト範囲
		constexpr RangeF DefaultRVecRange{-DefaultRVecValue, DefaultRVecValue};

		//! 条件を満たすランダムベクトルを生成
		template <class VT, class RD, class Chk>
		auto GenRVecCnd(RD& rd, Chk&& chk, const RangeF& r=DefaultRVecRange) {
			VT v;
			do {
				v = GenRVec<VT>(rd, r);
			} while(!std::forward<Chk>(chk)(v));
			return v;
		}
		//! ランダムなベクトル（但し長さがゼロではない）
		template <class VT, class RD>
		auto GenRVecLen(RD& rd, float th, const RangeF& r=DefaultRVecRange) {
			return GenRVecCnd<VT>(rd, [th](auto& v){ return v.length() >= th; }, r);
		}
		//! ランダムなベクトル (但し全ての成分の絶対値がそれぞれ基準範囲内)
		template <class VT, class RD>
		auto GenRVecAbs(RD& rd, const RangeF& rTh, const RangeF& r=DefaultRVecRange) {
			return GenRVecCnd<VT>(rd,
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
		template <class VT, class RD>
		auto GenRDir(RD& rd) {
			return GenRVecLen<VT>(rd, 1e-4f, {-1.f, 1.f}).normalization();
		}
	}
}
