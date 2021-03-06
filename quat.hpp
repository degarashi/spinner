#if !BOOST_PP_IS_ITERATING
	#if !defined(QUAT_H_) || INCLUDE_QUAT_LEVEL >= 1
		#define QUAT_H_
		#include "matrix.hpp"
		#include "error.hpp"
		#include "structure/angle.hpp"
		#include "tostring.hpp"
		#include "random/quat.hpp"
		#include <boost/serialization/access.hpp>
		#include <boost/serialization/level.hpp>
		// 要求された定義レベルを実体化
		#ifndef INCLUDE_QUAT_LEVEL
			#define INCLUDE_QUAT_LEVEL 0
		#endif
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (0,1, "quat.hpp", INCLUDE_QUAT_LEVEL))
		#include BOOST_PP_ITERATE()
		#undef INCLUDE_QUAT_LEVEL
	#endif
#elif BOOST_PP_ITERATION_DEPTH() == 1
	#define ALIGN	BOOST_PP_ITERATION()
	#define ALIGNA	AFLAG(ALIGN)
	#define ALIGNB	BOOLNIZE(ALIGN)
	#define ALIGN16	BOOST_PP_IF(ALIGN, alignas(16), NOTHING)
	#define VEC3	BOOST_PP_CAT(ALIGNA, Vec3)
	#define MAT33	BOOST_PP_CAT(ALIGNA, Mat33)
	#define MAT44	BOOST_PP_CAT(ALIGNA, Mat44)
	#define QT		QuatT<ALIGNB>
	#define EQT		ExpQuatT<ALIGNB>

	#define DIM		4
	#include "local_macro.hpp"

	#if BOOST_PP_ITERATION_FLAGS() == 0
	namespace spn {
		template <>
		struct ALIGN16 QuatT<ALIGNB> : QuatBase {
			constexpr static int width = 4;
			constexpr static bool align = ALIGNB;
			union {
				struct {
					float	x,y,z,w;
				};
				float	m[4];
			};

			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int /*ver*/) {
				ar & BOOST_SERIALIZATION_NVP(m);
			}

			QuatT() = default;
			QuatT(const QuatT<false>& q);
			QuatT(const QuatT<true>& q);
			QuatT(_TagIdentity);
			QuatT(const Vec3& v, float fw);
			QuatT(float fx, float fy, float fz, float fw);
			reg128 loadPS() const;

			template <int M, int N, bool A>
			static QuatT FromMat(const MatT<M,N,A>& m);
			static QuatT FromAxisF(float f11, float f12, float f13,
								float f21, float f22, float f23,
								float f31, float f32, float f33);
			static QuatT FromAxis(const VEC3& xA, const VEC3& yA, const VEC3& zA);
			static QuatT FromMatAxis(const VEC3& xA, const VEC3& yA, const VEC3& zA);
			static QuatT RotationYPR(RadF yaw, RadF pitch, RadF roll);
			static QuatT RotationX(RadF ang);
			static QuatT RotationY(RadF ang);
			static QuatT RotationZ(RadF ang);
			static QuatT Rotation(const VEC3& axis, RadF ang);
			static QuatT LookAt(const VEC3& dir, const VEC3& up);
			static QuatT Rotation(const VEC3& from, const VEC3& to);
			static QuatT SetLookAt(AXIS_FLAG targetAxis, AXIS_FLAG baseAxis, const VEC3& baseVec, const VEC3& at, const VEC3& pos);
			// -------- Luaへのエクスポート用(static) --------
			static QuatT Lua_FromMat33(const MatT<3,3,false>& m);
			static QuatT Lua_FromMat44(const MatT<4,4,false>& m);
			static QuatT Lua_Rotation(const VEC3& axis, RadF ang);
			static QuatT Lua_RotationFromTo(const VEC3& from, const VEC3& to);

			// This *= Quat::rotateX(ang)
			void rotateX(RadF ang);
			void rotateY(RadF ang);
			void rotateZ(RadF ang);
			void rotate(const VEC3& axis, RadF ang);
			void identity();
			void normalize();
			void conjugate();
			void invert();
			void scale(float s);

			#define DEF_OP0(dummy,align,op)	QuatT& operator BOOST_PP_CAT(op,=) (const QuatT<BOOLNIZE(align)>& q); \
						QuatT operator op (const QuatT<BOOLNIZE(align)>& q) const; \
						QuatT&& operator op (QuatT<BOOLNIZE(align)>&& q) const;
			#define DEF_OPS(op) QuatT& operator BOOST_PP_CAT(op,=) (float s); \
						QuatT operator op (float s) const;
			#define DEF_OP(op)	BOOST_PP_REPEAT(2, DEF_OP0, op)
			BOOST_PP_REPEAT(2, DEF_OP0, +)
			BOOST_PP_REPEAT(2, DEF_OP0, -)
			BOOST_PP_REPEAT(2, DEF_OP0, *)
			DEF_OPS(*)
			DEF_OPS(/)
			#undef DEF_OP
			#undef DEF_OPS
			#undef DEF_OP0
			// -------- Luaへのエクスポート用 --------
			QuatT addQ(const QuatT& q) const;
			QuatT subQ(const QuatT& q) const;
			QuatT mulQ(const QuatT& q) const;
			QuatT mulF(float s) const;
			QuatT divF(float s) const;
			bool equal(const QuatT& q) const;
			std::string toString() const;

			// return This * Quat::rotateX(ang)
			QuatT rotationX(RadF ang) const;
			QuatT rotationY(RadF ang) const;
			QuatT rotationZ(RadF ang) const;
			QuatT rotation(const VEC3& axis, RadF ang) const;
			QuatT normalization() const;
			QuatT conjugation() const;
			QuatT inverse() const;
			float len_sq() const;
			float length() const;
			RadF angle() const;
			VEC3 getVector() const;
			VEC3 getAxis() const;

			float dot(const QuatT& q) const;
			bool operator == (const QuatT& q) const;
			bool operator != (const QuatT& q) const;
			QuatT operator >> (const QuatT& q) const;
			QuatT& operator >>= (const QuatT& q);
			QuatT slerp(const QuatT& q, float t) const;

			VEC3 getXAxis() const;			//!< 回転を行列表現した時のX軸
			VEC3 getXAxisInv() const;		//!< 正規直行座標に回転を掛けた後のX軸
			VEC3 getYAxis() const;			//!< 回転を行列表現した時のY軸
			VEC3 getYAxisInv() const;		//!< 正規直行座標に回転を掛けた後のY軸
			VEC3 getZAxis() const;			//!< 回転を行列表現した時のZ軸
			VEC3 getZAxisInv() const;		//!< 正規直行座標に回転を掛けた後のZ軸
			VEC3 getRight() const;			//!< X軸に回転を適用したベクトル
			VEC3 getUp() const;				//!< Y軸に回転を適用したベクトル
			VEC3 getDir() const;			//!< Z軸に回転を適用したベクトル

			float distance(const QuatT& q) const;

			// 行列変換
			MAT33 asMat33() const;
			MAT44 asMat44() const;
			EQT asExpQuat() const;

			template <class RDF>
			static QuatT Random(const RDF& rdf) {
				return random::GenRQuat<QuatT>(rdf);
			}

			friend std::ostream& operator << (std::ostream&, const QuatT&);
		};
		using BOOST_PP_CAT(ALIGNA, Quat) = QT;
	}
	BOOST_CLASS_IMPLEMENTATION(spn::QT, object_serializable)
	#elif BOOST_PP_ITERATION_FLAGS() == 1
	namespace spn {
		std::ostream& operator << (std::ostream& os, const QT& q) {
			return os << "Quat: [" << q.x << ", " << q.y << ", " << q.z << ", " << q.w << ']';
		}
		QT::QuatT(const QuatT<false>& q) { STORETHIS(LOADPSU(q.m)); }
		QT::QuatT(const QuatT<true>& q) { STORETHIS(LOADPS(q.m)); }
		QT::QuatT(_TagIdentity) { STORETHIS(reg_setr_ps(0,0,0,1)); }
		QT::QuatT(const Vec3& v, float fw) {
			x=v.x; y=v.y; z=v.z;
			w = fw;
		}
		QT::QuatT(float fx, float fy, float fz, float fw) {
			x = fx;
			y = fy;
			z = fz;
			w = fw;
		}
		reg128 QT::loadPS() const { return LOADTHIS(); }
		QT QT::FromAxis(const VEC3& xA, const VEC3& yA, const VEC3& zA) {
			return FromAxisF(xA.x, xA.y, xA.z,
							yA.x, yA.y, yA.z,
							zA.x, zA.y, zA.z);
		}
		QT QT::FromMatAxis(const VEC3& xA, const VEC3& yA, const VEC3& zA) {
			return FromAxisF(xA.x, yA.x, zA.x,
							xA.y, yA.y, zA.y,
							xA.z, yA.z, zA.z);
		}
		QT QT::FromAxisF(float f11, float f12, float f13,
								float f21, float f22, float f23,
								float f31, float f32, float f33)
		{
			// 最大成分を検索
			const float elem[4] = {f11 - f22 - f33 + 1.0f,
									-f11 + f22 - f33 + 1.0f,
									-f11 - f22 + f33 + 1.0f,
									f11 + f22 + f33 + 1.0f};
			int idx = 0;
			for(int i=1 ; i<4 ; i++) {
				if(elem[i] > elem[idx])
					idx = i;
			}
			Assert(Trap, elem[idx] >= 0, "invalid matrix error")

			// 最大要素の値を算出
			QuatT res;
			float* qr = reinterpret_cast<float*>(&res);
			float v = std::sqrt(elem[idx]) * 0.5f;
			qr[idx] = v;
			float mult = 0.25f / v;
			switch(idx) {
				case 0: // x
					qr[1] = (f12 + f21) * mult;
					qr[2] = (f31 + f13) * mult;
					qr[3] = (f23 - f32) * mult;
					break;
				case 1: // y
					qr[0] = (f12 + f21) * mult;
					qr[2] = (f23 + f32) * mult;
					qr[3] = (f31 - f13) * mult;
					break;
				case 2: // z
					qr[0] = (f31 + f13) * mult;
					qr[1] = (f23 + f32) * mult;
					qr[3] = (f12 - f21) * mult;
					break;
				case 3: // w
					qr[0] = (f23 - f32) * mult;
					qr[1] = (f31 - f13) * mult;
					qr[2] = (f12 - f21) * mult;
					break;
			}
			return res;
		}
		template <int M, int N, bool A>
		QT QT::FromMat(const MatT<M,N,A>& m) {
			return FromAxisF(m.ma[0][0], m.ma[0][1], m.ma[0][2],
							 m.ma[1][0], m.ma[1][1], m.ma[1][2],
							m.ma[2][0], m.ma[2][1], m.ma[2][2]);
		}
		#define DEF_FROMMAT1(z,n1,n0)	template QT QT::FromMat(const MatT<n0,n1,true>&); \
										template QT QT::FromMat(const MatT<n0,n1,false>&);
		#define DEF_FROMMAT0(z,n0,dummy)	BOOST_PP_REPEAT_FROM_TO_##z(3, 5, DEF_FROMMAT1, n0)
		BOOST_PP_REPEAT_FROM_TO(3,5, DEF_FROMMAT0, NOTHING)

		QT QT::Lua_FromMat33(const MatT<3,3,false>& m) {
			return FromMat(m);
		}
		QT QT::Lua_FromMat44(const MatT<4,4,false>& m) {
			return FromMat(m);
		}
		QT QT::Lua_Rotation(const VEC3& axis, RadF ang) {
			return Rotation(axis, ang);
		}
		QT QT::Lua_RotationFromTo(const VEC3& from, const VEC3& to) {
			return Rotation(from, to);
		}

		void QT::identity() {
			STORETHIS(reg_setr_ps(0,0,0,1));
		}
		void QT::conjugate() {
			STORETHIS(reg_mul_ps(LOADTHIS(), reg_setr_ps(-1,-1,-1,1)));
		}
		QT QT::conjugation() const {
			QuatT ret;
			STORETHISPS(ret.m, reg_mul_ps(LOADTHIS(), reg_setr_ps(-1,-1,-1,1)));
			return ret;
		}
		void QT::invert() {
			conjugate();
			*this *= spn::Rcp22Bit(len_sq());
		}
		QT QT::inverse() const {
			QuatT q(*this);
			q.invert();
			return q;
		}
		void QT::scale(float s) {
			QuatT q(TagIdentity);
			q.slerp(*this, s);
			*this = q;
		}
		float QT::len_sq() const {
			reg128 xm = LOADTHIS();
			xm = reg_mul_ps(xm, xm);
			SUMVEC(xm);

			float ret;
			reg_store_ss(&ret, xm);
			return ret;
		}
		float QT::length() const {
			return spn::Sqrt(len_sq());
		}
		void QT::normalize() {
			*this = normalization();
		}
		QT QT::normalization() const {
			float rlen = spn::Rcp22Bit(length());
			QuatT q;
			STORETHISPS(q.m, reg_mul_ps(LOADTHIS(), reg_load1_ps(&rlen)));
			return q;
		}
		#define DEF_OP0(align, op, func)	QT& QT::operator BOOST_PP_CAT(op,=) (const QuatT<BOOLNIZE(align)>& q) { \
			STORETHIS(func(LOADTHIS(), BOOST_PP_CAT(BOOST_PP_CAT(LOADPS_, ALIGNA), 4)(q.m))); return *this; } \
			QT QT::operator op (const QuatT<BOOLNIZE(align)>& q) const { \
				QT tq(*this); tq BOOST_PP_CAT(op,=) q; return tq; } \
			QT&& QT::operator op (QuatT<BOOLNIZE(align)>&& q) const { \
				STORETHISPS(q.m, func(LOADTHIS(), BOOST_PP_CAT(BOOST_PP_CAT(LOADPS_, ALIGNA), 4)(q.m))); return std::forward<QT>(q); }

		#define DEF_OP1(z,align,ops)	DEF_OP0(align, BOOST_PP_TUPLE_ELEM(0,ops), BOOST_PP_TUPLE_ELEM(1,ops))
		#define DEF_OP(op, func)		BOOST_PP_REPEAT(2, DEF_OP1, (op,func))
		DEF_OP(+, reg_add_ps)
		DEF_OP(-, reg_sub_ps)
		#undef DEF_OP1
		#undef DEF_OP0

		QT& QT::operator *= (float s) {
			STORETHIS(reg_mul_ps(LOADTHIS(), reg_load1_ps(&s)));
			return *this;
		}
		QT QT::operator * (float s) const {
			QuatT q;
			STORETHISPS(q.m, reg_mul_ps(LOADTHIS(), reg_load1_ps(&s)));
			return q;
		}
		QT& QT::operator /= (float s) { return *this *= spn::Rcp22Bit(s); }
		QT QT::operator / (float s) const { return *this * spn::Rcp22Bit(s); }

		QT& QT::operator *= (const QuatT& q) {
			QuatT t(w*q.x + x*q.w + y*q.z - z*q.y,
					w*q.y - x*q.z + y*q.w + z*q.x,
					w*q.z + x*q.y - y*q.x + z*q.w,
					w*q.w - x*q.x - y*q.y - z*q.z);
			return *this = t;
		}
		QT QT::operator * (const QuatT& q) const {
			QuatT tq(*this);
			return tq *= q;
		}
		QT&& QT::operator * (QuatT&& q) const {
			// TODO: rvalueに最適化
			QuatT tq(*this);
			tq *= q;
			q = tq;
			return std::forward<QuatT>(q);
		}
		bool QT::operator == (const QuatT& q) const {
			using VT = VecT<4,ALIGNB>;
			return reinterpret_cast<const VT&>(*this) == reinterpret_cast<const VT&>(q);
		}
		bool QT::operator != (const QuatT& q) const {
			return !(this->operator == (q));
		}

		QT QT::addQ(const QuatT& q) const {
			return *this + q;
		}
		QT QT::subQ(const QuatT& q) const {
			return *this - q;
		}
		QT QT::mulQ(const QuatT& q) const {
			return *this * q;
		}
		QT QT::mulF(float s) const {
			return *this * s;
		}
		QT QT::divF(float s) const {
			return *this / s;
		}
		bool QT::equal(const QuatT& q) const {
			return *this == q;
		}
		std::string QT::toString() const {
			return ToString(*this);
		}

		/*
		#define DEF_OP00(align, op, func)	QT& QT::operator BOOST_PP_CAT(op,=) (const QuatT<BOOLNIZE(align)>& q) {
		#define DEF_OP()
		DEF_OPS(*, reg_mul_ps)
		DEF_OPS(/, _mmDivPs)
		*/
		QT QT::slerp(const QuatT& q, float t) const {
			const float ac = Saturate(dot(q), 0.f, 1.f);
			const float theta = std::acos(ac),
						S = std::sin(theta);
			if(std::fabs(S) < 1e-4f)
				return *this;
			QuatT rq = *this * (std::sin(theta*(1-t)) / S);
			rq += q * (std::sin(theta * t) / S);
			return rq;
		}
		QT QT::Rotation(const VEC3& axis, RadF ang) {
			float C=std::cos(ang.get()/2),
				S=std::sin(ang.get()/2);
			auto taxis = axis * S;
			return QT(taxis.x, taxis.y, taxis.z, C);
		}
		QT QT::RotationX(RadF ang) {
			ang *= 0.5f;
			return QuatT(std::sin(ang.get()), 0, 0, std::cos(ang.get()));
		}
		QT QT::RotationY(RadF ang) {
			ang *= 0.5f;
			return QuatT(0, std::sin(ang.get()), 0, std::cos(ang.get()));
		}
		QT QT::RotationZ(RadF ang) {
			ang *= 0.5f;
			return QuatT(0, 0, std::sin(ang.get()), std::cos(ang.get()));
		}

		namespace {
			spn::Optional<std::pair<VEC3, RadF>> GetRotAxis(const VEC3& from, const VEC3& to) {
				float d = from.dot(to);
				if(d > 1.0f - 1e-4f)
					return spn::none;
				if(d < -1.0f + 1e-4f) {
					VEC3 axis(1,0,0);
					if(std::fabs(axis.dot(from)) > 1.0f-1e-3f)
						axis = VEC3(0,1,0);
					return std::make_pair(axis, RadF(PI));
				}
				VEC3 axis = from.cross(to);
				axis.normalize();
				RadF ang(std::acos(d));
				return std::make_pair(axis, ang);
			}
		}
		QT QT::LookAt(const VEC3& dir, const VEC3& up) {
			VEC3 t_up = up;
			VEC3 rv = t_up % dir;
			float len_s = rv.len_sq();
			if(len_s < 1e-6f) {
				// 真上か真下を向いている
				// upベクトルは適当に定める
				t_up = dir.verticalVector();
				rv = t_up % dir;
			} else {
				rv *= RSqrt(len_s);
				t_up = dir % rv;
			}
			return FromAxis(rv, t_up, dir);
		}
		QT QT::SetLookAt(AXIS_FLAG targetAxis, AXIS_FLAG baseAxis, const VEC3& baseVec, const VEC3& at, const VEC3& pos) {
			// [0] = target
			// [1] = right
			// [2] = base
			int axF[3] = {targetAxis, 0, baseAxis};
			// target, base以外の軸を設定
			switch((1<<targetAxis) | (1<<baseAxis)) {
				// 110
				case 0x06:
					axF[1] = AXIS_X;
					break;
				// 101
				case 0x05:
					axF[1] = AXIS_Y;
					break;
				// 011
				case 0x03:
					axF[1] = AXIS_Z;
					break;
				default:
					Assert(Trap, false, "invalid axis flag")
			}

			VEC3 axis[3];
			auto &vTarget = axis[axF[0]],
				&vOther = axis[axF[1]],
				&vBase = axis[axF[2]];
			vTarget = at - pos;
			if(vTarget.normalize() < 1e-6f)
				return QT(0,0,0,1);
			vOther = baseVec.cross(vTarget);
			if(vOther.normalize() < 1e-6f)
				return QT(0,0,0,1);
			vBase = vTarget.cross(vOther);
			return FromAxis(axis[0], axis[1], axis[2]);
		}
		QT QT::Rotation(const VEC3& from, const VEC3& to) {
			VEC3 rAxis = from % to;
			if(rAxis.len_sq() < 1e-4f)
				return QuatT(0,0,0,1);
			rAxis.normalize();
			float d = Saturate(from.dot(to), 1.f);
			d = std::acos(d);
			return Rotation(rAxis, RadF(d));
		}

		void QT::rotateX(RadF ang) {
			*this = RotationX(ang) * *this;
		}
		void QT::rotateY(RadF ang) {
			*this = RotationY(ang) * *this;
		}
		void QT::rotateZ(RadF ang) {
			*this = RotationZ(ang) * *this;
		}
		void QT::rotate(const VEC3& axis, RadF ang) {
			*this = Rotation(axis, ang) * *this;
		}

		QT QT::rotationX(RadF ang) const {
			return RotationX(ang) * *this;
		}
		QT QT::rotationY(RadF ang) const {
			return RotationY(ang) * *this;
		}
		QT QT::rotationZ(RadF ang) const {
			return RotationZ(ang) * *this;
		}
		QT QT::rotation(const VEC3& axis, RadF ang) const {
			return Rotation(axis, ang) * *this;
		}
		QT QT::operator >> (const QuatT& q) const {
			return q * *this;
		}
		QT& QT::operator >>= (const QuatT& q) {
			return *this = (q * *this);
		}
		float QT::dot(const QuatT& q) const {
			return x*q.x + y*q.y + z*q.z + w*q.w;
		}
		float QT::distance(const QuatT& q) const {
			// 単純に値の差分をとる
			float f[4] = {x - q.x,
						y - q.y,
						z - q.z,
						w - q.w};
			float sum = 0;
			for(int i=0 ; i<4 ; i++)
				sum += std::fabs(f[i]);
			return sum;
		}
		RadF QT::angle() const {
			return RadF(std::acos(Saturate(w, 1.f))*2);
		}
		VEC3 QT::getVector() const {
			return VEC3(x,y,z);
		}
		VEC3 QT::getAxis() const {
			float s_theta = std::sqrt(1.0f - Square(w));
			if(s_theta < 1e-6f)
				return Vec3(0,0,0);
			s_theta = 1.0f / s_theta;
			return VEC3(x*s_theta, y*s_theta, z*s_theta);
		}

		#define ELEM00 (1-2*y*y-2*z*z)
		#define ELEM01 (2*x*y+2*w*z)
		#define ELEM02 (2*x*z-2*w*y)
		#define ELEM10 (2*x*y-2*w*z)
		#define ELEM11 (1-2*x*x-2*z*z)
		#define ELEM12 (2*y*z+2*w*x)
		#define ELEM20 (2*x*z+2*w*y)
		#define ELEM21 (2*y*z-2*w*x)
		#define ELEM22 (1-2*x*x-2*y*y)
		MAT33 QT::asMat33() const {
			MAT33 ret(ELEM00, ELEM01, ELEM02,
						ELEM10, ELEM11, ELEM12,
						ELEM20, ELEM21, ELEM22);
			return ret;
		}
		MAT44 QT::asMat44() const {
			MAT44 ret(ELEM00, ELEM01, ELEM02, 0,
						ELEM10, ELEM11, ELEM12, 0,
						ELEM20, ELEM21, ELEM22, 0,
						0,0,0,1);
			return ret;
		}
		EQT QT::asExpQuat() const {
			return EQT(*this);
		}
		VEC3 QT::getXAxis() const {
			return VEC3(ELEM00, ELEM10, ELEM20); }
		VEC3 QT::getXAxisInv() const {
			return VEC3(ELEM00, ELEM01, ELEM02); }
		VEC3 QT::getYAxis() const {
			return VEC3(ELEM01, ELEM11, ELEM21); }
		VEC3 QT::getYAxisInv() const {
			return VEC3(ELEM10, ELEM11, ELEM12); }
		VEC3 QT::getZAxis() const {
			return VEC3(ELEM02, ELEM12, ELEM22); }
		VEC3 QT::getZAxisInv() const {
			return VEC3(ELEM20, ELEM21, ELEM22); }
		VEC3 QT::getRight() const {
			return VEC3(ELEM00, ELEM01, ELEM02); }
		VEC3 QT::getUp() const {
			return VEC3(ELEM10, ELEM11, ELEM12); }
		VEC3 QT::getDir() const {
			return VEC3(ELEM20, ELEM21, ELEM22); }
		QT QT::RotationYPR(RadF yaw, RadF pitch, RadF roll) {
			QuatT q(0,0,0,1);
			// roll
			q.rotateZ(roll);
			// pitch
			q.rotateX(-pitch);
			// yaw
			q.rotateY(yaw);
			return q;
		}
	}
	#endif
	#include "local_unmacro.hpp"
	#undef ALIGN
	#undef ALIGNA
	#undef ALIGNB
	#undef ALIGN16
	#undef VEC3
	#undef MAT33
	#undef MAT44
	#undef QT
	#undef EQT
	#undef DIM
#endif
