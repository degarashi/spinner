#if !BOOST_PP_IS_ITERATING
	#if !defined(QUAT_H_) || INCLUDE_QUAT_LEVEL >= 1
		#define QUAT_H_
		#include "matrix.hpp"
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

	namespace spn {
	#if BOOST_PP_ITERATION_FLAGS() == 0
		template <>
		struct QuatT<ALIGNB> : QuatBase {
			union {
				float	x,y,z,w;
				float	m[4];
			};
			
			QuatT() = default;
			QuatT(const QuatT<false>& q);
			QuatT(const QuatT<true>& q);
			QuatT(_TagIdentity);
			QuatT(float fx, float fy, float fz, float fw);
			__m128 loadPS() const;
			
			template <int M, int N, bool A>
			static QuatT FromMat(const MatT<M,N,A>& m);
			static QuatT FromAxisF(float f11, float f12, float f13,
								float f21, float f22, float f23,
								float f31, float f32, float f33);
			static QuatT FromAxis(const VEC3& xA, const VEC3& yA, const VEC3& zA);
			static QuatT RotationYPR(float yaw, float pitch, float roll);
			static QuatT RotationX(float ang);
			static QuatT RotationY(float ang);
			static QuatT RotationZ(float ang);
			static QuatT Rotation(const VEC3& axis, float ang);
			static QuatT LookAtLH(const VEC3& dir);
			static QuatT Rotation(const VEC3& from, const VEC3& to);
			static QuatT SetLookAt(AXIS_FLAG targetAxis, AXIS_FLAG baseAxis, const VEC3& baseVec, const VEC3& at, const VEC3& pos);
			
			// This *= Quat::rotateX(ang)
			void rotateX(float ang);
			void rotateY(float ang);
			void rotateZ(float ang);
			void rotate(const VEC3& axis, float ang);
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
			
			// return This * Quat::rotateX(ang)
			QuatT rotationX(float ang) const;
			QuatT rotationY(float ang) const;
			QuatT rotationZ(float ang) const;
			QuatT rotation(const VEC3& axis, float ang) const;
			QuatT normalization() const;
			QuatT conjugation() const;
			QuatT inverse() const;
			float len_sq() const;
			float length() const;
			float angle() const;
			VEC3 getAxis() const;

			float dot(const QuatT& q) const;
			QuatT& operator >>= (const QuatT& q);
			QuatT lerp(const QuatT& q, float t) const;
			
			VEC3 getXAxis() const;
			VEC3 getYAxis() const;
			VEC3 getZAxis() const;
			
			float distance(const QuatT& q) const;

			// 行列変換
			MAT33 asMat33() const;
			MAT44 asMat44() const;
			EQT asExpQuat() const;
		};
		using BOOST_PP_CAT(ALIGNA, Quat) = QT;
	#elif BOOST_PP_ITERATION_FLAGS() == 1
		QT::QuatT(const QuatT<false>& q) { STORETHIS(LOADPSU(q.m)); }
		QT::QuatT(const QuatT<true>& q) { STORETHIS(LOADPS(q.m)); }
		QT::QuatT(_TagIdentity) { STORETHIS(_mm_setr_ps(0,0,0,1)); }
		QT::QuatT(float fx, float fy, float fz, float fw) {
			x = fx;
			y = fy;
			z = fz;
			w = fw;
		}
		__m128 QT::loadPS() const { return LOADTHIS(); }
		QT QT::FromAxis(const VEC3& xA, const VEC3& yA, const VEC3& zA) {
			return FromAxisF(xA.x, xA.y, xA.z,
						yA.x, yA.y, yA.z,
						zA.x, zA.y, zA.z);
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
			if(elem[idx] >= 0)
				throw std::runtime_error("invalid matrix error");
				
			// 最大要素の値を算出
			QuatT res;
			float* qr = reinterpret_cast<float*>(&res);
			float v = std::sqrtf(elem[idx]) * 0.5f;
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
		
		void QT::identity() {
			STORETHIS(_mm_setr_ps(0,0,0,1));
		}
		void QT::conjugate() {
			STORETHIS(_mm_mul_ps(LOADTHIS(), _mm_setr_ps(-1,-1,-1,1)));
		}
		QT QT::conjugation() const {
			QuatT ret;
			STORETHISPS(ret.m, _mm_mul_ps(LOADTHIS(), _mm_setr_ps(-1,-1,-1,1)));
			return ret;
		}
		void QT::invert() {
			conjugate();
			*this *= _sseRcp22Bit(len_sq());
		}
		QT QT::inverse() const {
			QuatT q(*this);
			q.invert();
			return q;
		}
		void QT::scale(float s) {
			QuatT q(TagIdentity);
			q.lerp(*this, s);
			*this = q;
		}
		float QT::len_sq() const {
			__m128 xm = LOADTHIS();
			xm = _mm_mul_ps(xm, xm);
			SUMVEC(xm);
			
			float ret;
			_mm_store_ss(&ret, xm);
			return ret;
		}
		float QT::length() const {
			return _sseSqrt(len_sq());
		}
		void QT::normalize() {
			*this = normalization();
		}
		QT QT::normalization() const {
			float rlen = _sseRcp22Bit(length());
			QuatT q;
			STORETHISPS(q.m, _mm_mul_ps(LOADTHIS(), _mm_load1_ps(&rlen)));
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
		DEF_OP(+, _mm_add_ps)
		DEF_OP(-, _mm_sub_ps)
		
		QT& QT::operator *= (float s) {
			STORETHIS(_mm_mul_ps(LOADTHIS(), _mm_load1_ps(&s)));
			return *this;
		}
		QT QT::operator * (float s) const {
			QuatT q;
			STORETHISPS(q.m, _mm_mul_ps(LOADTHIS(), _mm_load1_ps(&s)));
			return q;
		}
		QT& QT::operator /= (float s) { return *this *= _sseRcp22Bit(s); }
		QT QT::operator / (float s) const { return *this * _sseRcp22Bit(s); }
		
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

		/*
		#define DEF_OP00(align, op, func)	QT& QT::operator BOOST_PP_CAT(op,=) (const QuatT<BOOLNIZE(align)>& q) {
		#define DEF_OP()
		DEF_OPS(*, _mm_mul_ps)
		DEF_OPS(/, _mmDivPs)
		*/
		QT QT::lerp(const QuatT& q, float t) const {
			float theta = std::acos(dot(q));
			QuatT rq = *this * (std::sin(theta*(1-t)) / std::sin(theta));
			rq += q * (std::sin(theta * t) / std::sin(theta));
			return rq;
		}
		QT QT::Rotation(const VEC3& axis, float ang) {
			float C=std::cos(ang/2),
				S=std::sin(ang/2);
			auto taxis = axis * S;
			return QT(taxis.x, taxis.y, taxis.z, C);
		}
		QT QT::RotationX(float ang) {
			ang *= 0.5f;
			return QuatT(std::sin(ang), 0, 0, std::cos(ang));
		}
		QT QT::RotationY(float ang) {
			ang *= 0.5f;
			return QuatT(0, std::sin(ang), 0, std::cos(ang));
		}
		QT QT::RotationZ(float ang) {
			ang *= 0.5f;
			return QuatT(0, 0, std::sin(ang), std::cos(ang));
		}

		namespace {
			std::tuple<VEC3,float,bool> GetRotAxis(const VEC3& from, const VEC3& to) {
				float d = from.dot(to);
				if(d > 1.0f - 1e-4f)
					return std::make_tuple(VEC3(0,0,0), 0.0f, false);
				if(d < -1.0f + 1e-4f) {
					VEC3 axis(1,0,0);
					if(std::fabs(axis.dot(from)) > 1.0f-1e-3f)
						axis = VEC3(0,1,0);
					return std::make_tuple(axis, float(PI), true);
				}
				VEC3 axis = from.cross(to);
				axis.normalize();
				float ang = acos(d);
				return std::make_tuple(axis, ang, true);
			}
		}
		QT QT::LookAtLH(const VEC3& dir) {
			VEC3 axis;
			float ang, upang;
			auto ret0 = GetRotAxis(VEC3(0,0,1), dir);
			axis = std::get<0>(ret0);
			ang = std::get<1>(ret0);
			if(!std::get<2>(ret0))
				return QuatT(0,0,0,1);
			QuatT q1 = QuatT::Rotation(axis, ang);
			// UP軸を回転軸と合わせる
			VEC3 rD = VEC3(1,0,0) * q1;
			VEC3 rD2(rD.x, 0, rD.z);
			rD2.normalize();
			auto ret = GetRotAxis(rD, rD2);
			axis = std::get<0>(ret);
			upang = std::get<1>(ret);
			QuatT q0 = QuatT::Rotation(axis, upang);
			q1 >>= q0;
			return q1;
		}
		QT QT::SetLookAt(AXIS_FLAG targetAxis, AXIS_FLAG baseAxis, const VEC3& baseVec, const VEC3& at, const VEC3& pos) {
			int axF[3] = {targetAxis, 0, baseAxis};
			VEC3 axis[3];
			uint32_t flag = (1<<targetAxis) | (1<<baseAxis);
			switch(flag) {
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
					throw std::runtime_error("invalid axis flag");
			}

			axis[axF[0]] = at - pos;
			axis[axF[0]].normalize();
			axis[axF[1]] = baseVec.cross(axis[axF[0]]);
			axis[axF[1]].normalize();
			axis[axF[2]] = axis[axF[0]].cross(axis[axF[1]]);

			return LookAtLH(axis[2]);
		}
		QT QT::Rotation(const VEC3& from, const VEC3& to) {
			VEC3 rAxis = to % from;
			if(rAxis.len_sq() < 1e-4f)
				return QuatT(0,0,0,1);
			rAxis.normalize();
			float d = from.dot(to);
			d = acos(d);
			return Rotation(rAxis, d);
		}

		void QT::rotateX(float ang) {
			*this = RotationX(ang) * *this;
		}
		void QT::rotateY(float ang) {
			*this = RotationY(ang) * *this;
		}
		void QT::rotateZ(float ang) {
			*this = RotationZ(ang) * *this;
		}
		void QT::rotate(const VEC3& axis, float ang) {
			*this = Rotation(axis, ang) * *this;
		}

		QT QT::rotationX(float ang) const {
			return RotationX(ang) * *this;
		}
		QT QT::rotationY(float ang) const {
			return RotationY(ang) * *this;
		}
		QT QT::rotationZ(float ang) const {
			return RotationZ(ang) * *this;
		}
		QT QT::rotation(const VEC3& axis, float ang) const {
			return Rotation(axis, ang) * *this;
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
		float QT::angle() const {
			return std::acos(w)*2;
		}
		VEC3 QT::getAxis() const {
			float s_theta = std::sqrt(1.0f - Square(w));
			if(s_theta < 1e-6f)
				return Vec3(0,0,0);
			s_theta = 1.0f / s_theta;
			return VEC3(x*s_theta, y*s_theta, z*s_theta);
		}

		MAT33 QT::asMat33() const {
			MAT33 ret(1-2*y*y-2*z*z, 2*x*y-2*w*z, 2*x*z+2*w*y,
						2*x*y+2*w*z, 1-2*x*x-2*z*z, 2*y*z-2*w*x,
						2*x*z-2*w*y, 2*y*z+2*w*x, 1-2*x*x-2*y*y);
			return ret;
		}
		MAT44 QT::asMat44() const {
			MAT44 ret(1-2*y*y-2*z*z, 2*x*y-2*w*z, 2*x*z+2*w*y, 0,
						2*x*y+2*w*z, 1-2*x*x-2*z*z, 2*y*z-2*w*x, 0,
						2*x*z-2*w*y, 2*y*z+2*w*x, 1-2*x*x-2*y*y, 0,
						0,0,0,1);
			return ret;
		}
		EQT QT::asExpQuat() const {
			return EQT(*this);
		}
		VEC3 QT::getXAxis() const {
			return VEC3(1-2*y*y-2*z*z, 2*x*y+2*w*z, 2*x*z-2*w*y);
		}
		VEC3 QT::getYAxis() const {
			return VEC3(2*x*y-2*w*z, 1-2*x*x-2*z*z, 2*y*z+2*w*x);
		}
		VEC3 QT::getZAxis() const {
			return VEC3(2*x*z+2*w*y, 2*y*z-2*w*x, 1-2*x*x-2*y*y);
		}
		QT QT::RotationYPR(float yaw, float pitch, float roll) {
			QuatT q(0,0,0,1);
			// roll
			q.rotateZ(roll);
			// pitch
			q.rotateX(pitch);
			// yaw
			q.rotateY(yaw);
			return q;
		}
	#endif
	}
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
