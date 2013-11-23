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
	void GramSchmidtOrth(Vec3& v0, Vec3& v1, Vec3& v2) {
		v0.normalize();
		v1 -= v0 * v0.dot(v1);
		v1.normalize();
		v2 -= v0*v0.dot(v2) + v1*v1.dot(v2);
		v2.normalize();
	}
	AffineParts DecompAffine(const AMat43& m) {
		AMat33 tm;
		AffineParts ap;
		// オフセットは4行目をそのまま抽出
		ap.offset = m.getRow(3);
		AVec3 colm[3];
		for(int i=0 ; i<3 ; i++) {
			auto r = m.getRow(i);
			ap.scale.m[i] = r.length();
			tm.getRow(i) = r * Rcp22Bit(ap.scale.m[i]);
		}
		for(int i=0 ; i<3 ; i++)
			colm[i] = tm.getColumn(i);

		auto tmp = colm[0] % colm[1];
		ap.rotation = Quat::FromAxis(colm[0], colm[1], colm[2]);
		return ap;
	}
}
