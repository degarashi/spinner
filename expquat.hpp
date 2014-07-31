#if !BOOST_PP_IS_ITERATING
	#if !defined(EXPQUAT_H_) || INCLUDE_EXPQUAT_LEVEL >= 1
		#define EXPQUAT_H_
		#include "quat.hpp"
		// 要求された定義レベルを実体化
		#ifndef INCLUDE_EXPQUAT_LEVEL
			#define INCLUDE_EXPQUAT_LEVEL 0
		#endif
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (0,1, "expquat.hpp", INCLUDE_EXPQUAT_LEVEL))
		#include BOOST_PP_ITERATE()
		#undef INCLUDE_EXPQUAT_LEVEL
	#endif

#elif BOOST_PP_ITERATION_DEPTH() == 1
	#define ALIGN	BOOST_PP_ITERATION()
	#define ALIGNA	AFLAG(ALIGN)
	#define ALIGNB	BOOLNIZE(ALIGN)
	#define ALIGN16	BOOST_PP_IF(ALIGN, alignas(16), NOTHING)
	#define QT		QuatT<ALIGNB>
	#define EQT		ExpQuatT<ALIGNB>
	#define	VEC3	VecT<3, ALIGNB>

	#define DIM		3
	#include "local_macro.hpp"

	namespace spn {
	#if BOOST_PP_ITERATION_FLAGS() == 0
		template <>
		struct ALIGN16 ExpQuatT<ALIGNB> {
			constexpr static int width = 3;
			union {
				struct {
					float	x,y,z;
				};
				float	m[3];
			};

			ExpQuatT() = default;
			ExpQuatT(const ExpQuatT<false>& q);
			ExpQuatT(const ExpQuatT<true>& q);
			template <bool A>
			ExpQuatT(const QuatT<A>& q);
			ExpQuatT(float x, float y, float z);
			reg128 loadPS() const;
			QT asQuat() const;

			#define DEF_OP0(z,align,op)		ExpQuatT operator op (const ExpQuatT<BOOLNIZE(align)>& q) const; \
							ExpQuatT& operator BOOST_PP_CAT(op,=) (const ExpQuatT<BOOLNIZE(align)>& q);
			#define DEF_OP(op)	BOOST_PP_REPEAT(2, DEF_OP0, op)
			DEF_OP(+)
			DEF_OP(-)
			#undef DEF_OP0
			#undef DEF_OP
			#define DEF_OPS(op)	ExpQuatT operator op (float s) const; \
							ExpQuatT& operator BOOST_PP_CAT(op,=) (float s);
			DEF_OPS(*)
			DEF_OPS(/)
			#undef DEF_OPS
			#undef DEF_OP

			float len_sq() const;
			float length() const;
			const VEC3& asVec3() const;

			std::pair<float,VEC3> getAngAxis() const;
		};
		using BOOST_PP_CAT(ALIGNA, ExpQuat) = EQT;
	#else
		EQT::ExpQuatT(const ExpQuatT<false>& q) { STORETHIS(LOADPS_4(q.m)); }
		EQT::ExpQuatT(const ExpQuatT<true>& q) { STORETHIS(LOADPS_A4(q.m)); }
		template <bool A>
		EQT::ExpQuatT(const QuatT<A>& q) {
			if(std::fabs(q.w) >= 1.f-1e-6f) {
				// 無回転クォータニオンとみなす
				x = y = z = 0;
			} else {
				const float theta = std::acos(q.w);
				reg128 xm = reg_mul_ps(q.getAxis().loadPS(), reg_load1_ps(&theta));
				STORETHIS(xm);
			}
		}
		template EQT::ExpQuatT(const QuatT<false>&);
		template EQT::ExpQuatT(const QuatT<true>&);
		reg128 EQT::loadPS() const { return LOADTHIS(); }

		EQT::ExpQuatT(float fx, float fy, float fz) {
			x = fx;
			y = fy;
			z = fz;
		}
		QT EQT::asQuat() const {
			auto ret = getAngAxis();
			return QT::Rotation(ret.second, ret.first);
		}
		#define DEF_OP0(align,op,func)	EQT EQT::operator op (const ExpQuatT<BOOLNIZE(align)>& q) const { \
			EQT eq; \
			STORETHISPS(eq.m, func(LOADTHIS(), BOOST_PP_CAT(BOOST_PP_CAT(LOADPS_,ALIGNA),4)(q.m))); \
			return eq; } \
			EQT& EQT::operator BOOST_PP_CAT(op,=) (const ExpQuatT<BOOLNIZE(align)>& q) { \
			STORETHIS(func(LOADTHIS(), BOOST_PP_CAT(BOOST_PP_CAT(LOADPS_,ALIGNA),4)(q.m))); \
			return *this; }
		#define DEF_OP1(dummy,align,ops)	DEF_OP0(align,BOOST_PP_TUPLE_ELEM(0,ops), BOOST_PP_TUPLE_ELEM(1,ops))
		BOOST_PP_REPEAT(2,DEF_OP1,(+,reg_add_ps))
		BOOST_PP_REPEAT(2,DEF_OP1,(-,reg_sub_ps))
		#undef DEF_OP1
		#undef DEF_OP0

		EQT EQT::operator * (float s) const {
			EQT eq;
			STORETHISPS(eq.m, reg_mul_ps(LOADTHIS(), reg_load1_ps(&s)));
			return eq;
		}
		EQT& EQT::operator *= (float s) {
			STORETHIS(reg_mul_ps(LOADTHIS(), reg_load1_ps(&s)));
			return *this;
		}
		EQT EQT::operator / (float s) const {
			return *this * spn::Rcp22Bit(s);
		}
		EQT& EQT::operator /= (float s) {
			(*this) *= spn::Rcp22Bit(s);
			return *this;
		}
		const VEC3& EQT::asVec3() const {
			return *reinterpret_cast<const VEC3*>(this);
		}
		std::pair<float,VEC3> EQT::getAngAxis() const {
			auto axis = asVec3();
			float theta = axis.length();		// = (angle/2)
			if(theta < 1e-6f) {
				// 無回転クォータニオンとする
				return std::make_pair(0, VEC3(1,0,0));
			}
			axis /= theta;
			return std::make_pair(theta*2, axis);
		}
	#endif
	}
	#include "local_unmacro.hpp"
	#undef ALIGN
	#undef ALIGNA
	#undef ALIGNB
	#undef ALIGN16
	#undef QT
	#undef EQT
	#undef VEC3
	#undef DIM
#endif
