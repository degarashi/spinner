#pragma once
#include "rotation.hpp"
#include "vector.hpp"

namespace spn {
	namespace random {
		// --------------- quaternion ---------------
		//! ランダムなクォータニオン
		template <class QT, class RD>
		auto GenRQuat(RD& rd) {
			return QT::Rotation(GenRDir<AVec3>(rd), GenRAngle<RadF>(rd));
		}
		template <class EQT, class RD>
		auto GenRExpQuat(RD& rd) {
			return EQT(GenRQuat<QuatT<EQT::align>>(rd));
		}
	}
}
