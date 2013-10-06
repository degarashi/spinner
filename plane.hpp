
#if !BOOST_PP_IS_ITERATING
	#if !defined(PLANE_H_) || INCLUDE_PLANE_LEVEL >= 1
		#define PLANE_H_
		#include <tuple>

		// 要求された定義レベルを実体化
		#ifndef INCLUDE_PLANE_LEVEL
			#define INCLUDE_PLANE_LEVEL 0
		#endif
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (0,1, "plane.hpp", INCLUDE_PLANE_LEVEL))
		#include BOOST_PP_ITERATE()
		#undef INCLUDE_PLANE_LEVEL
	#endif
#elif BOOST_PP_ITERATION_DEPTH() == 1
	#define ALIGN	BOOST_PP_ITERATION()
	#define ALIGNA	AFLAG(ALIGN)
	#define ALIGNB	BOOLNIZE(ALIGN)
	#define ALIGN16	BOOST_PP_IF(ALIGN, alignas(16), NOTHING)
	#define VEC3	VecT<3,ALIGNB>
	#define PT		PlaneT<ALIGNB>

	#define DIM		4
	#include "local_macro.hpp"

	namespace spn {
	#if BOOST_PP_ITERATION_FLAGS() == 0
		template <>
		struct ALIGN16 PlaneT<ALIGNB> {
			using APlane = PlaneT<true>;
			using UPlane = PlaneT<false>;
			union {
				struct {
					float	a,b,c,d;
				};
				float	m[4];
			};

			PlaneT() = default;
			PlaneT(const UPlane& p);
			PlaneT(const APlane& p);
			PlaneT(float fa, float fb, float fc, float fd);
			PlaneT(const VEC3& orig, float dist);

			static PT FromPtDir(const VEC3& pos, const VEC3& dir);
			static PT FromPts(const VEC3& p0, const VEC3& p1, const VEC3& p2);
			static std::tuple<VEC3,bool> ChokePoint(const UPlane& p0, const UPlane& p1, const UPlane& p2);
			static std::tuple<VEC3,VEC3,bool> CrossLine(const UPlane& p0, const UPlane& p1);

			float dot(const VEC3& p) const;
			void move(float d);
			const VEC3& getNormal() const;
		};

		using BOOST_PP_CAT(ALIGNA, Plane) = PlaneT<ALIGNB>;
	#else
		PT::PlaneT(const UPlane& p) { STORETHIS(LOADPSU(p.m)); }
		PT::PlaneT(const APlane& p) { STORETHIS(LOADPS(p.m)); }
		PT::PlaneT(float fa, float fb, float fc, float fd) {
			a = fa;
			b = fb;
			c = fc;
			d = fd;
		}
		PT::PlaneT(const VEC3& orig, float dist) {
			STORETHIS(LOADTHISPS(orig.m));
			d = dist;
		}
		const VEC3& PT::getNormal() const {
			return *reinterpret_cast<const VEC3*>(this);
		}
		float PT::dot(const VEC3& p) const {
			reg128 xm = reg_mul_ps(LOADTHIS(), LOADPS_I3(p.m, 3));
			SUMVEC(xm)
			float ret;
			reg_store_ss(&ret, xm);
			return ret;
		}
		void PT::move(float fd) {
			d += fd;
		}
		PT PT::FromPtDir(const VEC3& pos, const VEC3& dir) {
			return PlaneT(dir, -dir.dot(pos));
		}
		PT PT::FromPts(const VEC3& p0, const VEC3& p1, const VEC3& p2) {
			VEC3 nml = (p1 - p0).cross(p2 - p0);
			nml.normalize();
			return PlaneT(nml, -p0.dot(nml));
		}
		//! 3つの平面が交差する座標を調べる
		std::tuple<VEC3,bool> PT::ChokePoint(const UPlane& p0, const UPlane& p1, const UPlane& p2) {
			AMat33 m, mInv;
			m.getRow(0) = p0.getNormal();
			m.getRow(1) = p1.getNormal();
			m.getRow(2) = p2.getNormal();
			m.transpose();

			if(!m.inversion(mInv))
				return std::make_tuple(VEC3(), false);

			VEC3 v(-p0.d, -p1.d, -p2.d);
			v *= mInv;
			return std::make_tuple(v, true);
		}
		//! 2つの平面が交差する直線を調べる
		std::tuple<VEC3,VEC3,bool> PT::CrossLine(const UPlane& p0, const UPlane& p1) {
			const auto &nml0 = p0.getNormal(),
						&nml1 = p1.getNormal();
			AVec3 nml = nml0 % nml1;
			if(std::fabs(nml.len_sq()) < FLOAT_EPSILON)
				return std::make_tuple(VEC3(),VEC3(), false);
			nml.normalize();

			AMat33 m, mInv;
			m.getRow(0) = nml0;
			m.getRow(1) = nml1;
			m.getRow(2) = nml;
			m.transpose();

			if(!m.inversion(mInv))
				return std::make_tuple(VEC3(),VEC3(),false);

			AVec3 v(-p0.d, -p1.d, 0);
			v *= mInv;
			return std::make_tuple(v, nml, true);
		}
	#endif
	}
	#include "local_unmacro.hpp"
	#undef ALIGN
	#undef ALIGNA
	#undef ALIGNB
	#undef ALIGN16
	#undef VEC3
	#undef PT
	#undef DIM
#endif
