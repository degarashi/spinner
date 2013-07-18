#include "misc.hpp"

namespace spn {
	float CramerDet(const Vec3& v0, const Vec3& v1, const Vec3& v2) {
		return v0.cross(v1).dot(v2);
	}
	float CramerDet(const Vec2& v0, const Vec2& v1) {
		return v0.x * v1.y - v1.x * v0.y;
	}

	Vec3 CramersRule(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& a0, float detInv) {
		return Vec3(CramerDet(a0,v1,v2) * detInv,
					CramerDet(v0,a0,v2) * detInv,
					CramerDet(v0,v1,a0) * detInv);
	}
	Vec2 CramersRule(const Vec2& v0, const Vec2& v1, const Vec2& a0, float detInv) {
		return Vec2(CramerDet(a0, v1) * detInv,
					CramerDet(v0, a0) * detInv);
	}
}
