#include "plane.hpp"
#include "matrix.hpp"

namespace spn {
	// ------------------------------ Plane ------------------------------
	Plane::Plane() {}
	Plane::Plane(float fa, float fb, float fc, float fd): a(fa), b(fb), c(fc), d(fd) {}
	Plane::Plane(const Vec3& orig, float dist): a(orig.x), b(orig.y), c(orig.z), d(dist) {}

	const Vec3& Plane::getNormal() const {
		return *reinterpret_cast<const Vec3*>(this);
	}
	float Plane::dot(const Vec3& p) const {
		__m128 xm = _mm_mul_ps(_mm_loadu_ps(&a), LOADPS_I3(p.m, 3));
		SUMVEC(xm)
		float ret;
		_mm_store_ss(&ret, xm);
		return ret;
	}
	void Plane::move(float fd) {
		d += fd;
	}

	Plane Plane::FromPtDir(const Vec3& pos, const Vec3& dir) {
		return Plane(dir.x, dir.y, dir.z, -dir.dot(pos));
	}
	Plane Plane::FromPts(const Vec3& p0, const Vec3& p1, const Vec3& p2) {
		Vec3 nml = Vec3(p1 - p0).cross(p2 - p0);
		nml.normalize();
		return Plane(nml.x, nml.y, nml.z, -p0.dot(nml));
	}
	//! 3つの平面が交差する座標を調べる
	std::tuple<Vec3,bool> Plane::ChokePoint(const Plane& p0, const Plane& p1, const Plane& p2) {
		AMat33 m, mInv;
		m.getRow(0) = p0.getNormal();
		m.getRow(1) = p1.getNormal();
		m.getRow(2) = p2.getNormal();
		m.transpose();
		
		if(!m.inversion(mInv))
			return std::make_tuple(Vec3(), false);

		AVec3 v(-p0.d, -p1.d, -p2.d);
		v *= mInv;

		return std::make_tuple(v, true);
	}
	//! 2つの平面が交差する直線を調べる
	std::tuple<Vec3,Vec3,bool> Plane::CrossLine(const Plane& p0, const Plane& p1) {
		const auto &nml0 = p0.getNormal(),
					&nml1 = p1.getNormal();
		Vec3 nml = nml0 % nml1;
		if(std::fabs(nml.len_sq()) < FLOAT_EPSILON)
			return std::make_tuple(Vec3(),Vec3(), false);
		nml.normalize();
		
		AMat33 m, mInv;
		m.getRow(0) = nml0;
		m.getRow(1) = nml1;
		m.getRow(2) = nml;
		m.transpose();
		
		if(!m.inversion(mInv))
			return std::make_tuple(Vec3(),Vec3(),false);

		AVec3 v(-p0.d, -p1.d, 0);
		v *= mInv;
		return std::make_tuple(v, nml, true);
	}
}
