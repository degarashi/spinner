#pragma once
#include "rotation.hpp"
#include "vector.hpp"

namespace spn {
	namespace random {
		// --------------- quaternion ---------------
		//! ランダムなクォータニオン
		template <class QT, class RDF>
		auto GenRQuat(const RDF& rdf) {
			return QT::Rotation(GenRDir<AVec3>(rdf), GenRAngle<RadF>(rdf));
		}
		template <class EQT, class RDF>
		auto GenRExpQuat(const RDF& rdf) {
			return EQT(GenRQuat<QuatT<EQT::align>>(rdf));
		}
	}
}
