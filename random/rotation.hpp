#pragma once

namespace spn {
	namespace random {
		// --------------- rotation ---------------
		//! ランダムな角度
		template <class ANG, class RD>
		ANG GenRAngle(RD& rd) {
			// キッチリ収まるようにする為の微量な補正値
			constexpr float Adjust = 1e-4f;
			return ANG(rd.template getUniform<float>({-ANG::OneRotationAng/2+Adjust,
						ANG::OneRotationAng/2-Adjust}));
		}
		//! 0から1のランダム値(float)
		template <class RD>
		auto GenR01(RD& rd) {
			return rd.template getUniform<float>({0.f, 1.f});
		}
	}
}
