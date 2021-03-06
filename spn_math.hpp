#pragma once
#include "macro.hpp"
#include "myintrin.hpp"
#include <boost/serialization/access.hpp>
#include <boost/serialization/traits.hpp>
#include <boost/serialization/nvp.hpp>
#include <functional>

#if !defined(SSE_LEVEL) || SSE_LEVEL <= 2
	#define SUMVEC(r)	{ reg128 tmp = reg_shuffle_ps(r, r, _REG_SHUFFLE(0,1,2,3)); \
		tmp = reg_add_ps(r, tmp); \
		r = reg_shuffle_ps(tmp,tmp, _REG_SHUFFLE(1,0,0,1)); \
		r = reg_add_ps(r, tmp); }
#else
	// rの各要素を足し合わせる
	#define SUMVEC(r)	{ r = reg_hadd_ps(r,r); \
		r = reg_hadd_ps(r,r); }
#endif
#define RCP22BIT(r) { reg128 tmprcp = reg_rcp_ps(r); \
	r = reg_mul_ps(r, reg_mul_ps(tmprcp, tmprcp)); \
	tmprcp = reg_add_ps(tmprcp,tmprcp); \
	r = reg_sub_ps(tmprcp, r); }

#include <cmath>
#include <cstdint>
namespace spn {
//! ニュートン法で逆数を計算
inline reg128 Rcp22Bit(reg128 r) {
	reg128 tmp(reg_rcp_ps(r));
	r = reg_mul_ps(r, reg_mul_ps(tmp,tmp));
	tmp = reg_add_ps(tmp,tmp);
	return reg_sub_ps(tmp, r);
}
//! ニュートン法で逆数を計算して積算 = reg_div_psよりはマシな除算
inline reg128 _mmDivPs(reg128 r0, reg128 r1) {
	return reg_mul_ps(r0, Rcp22Bit(r1));
}
inline reg128 _mmAbsPs(reg128 r) {
	const static reg128 signMask = reg_set1_ps(-0.0f);
	return reg_andnot_ps(signMask, r);
}
inline reg128 _mmSetPs(float w, float z, float y, float x) {
	const float tmp[4]{w,z,y,x};
	return reg_loadu_ps(tmp);
}
inline reg128 _mmSetPs(float s) {
	return reg_load1_ps(&s);
}
inline reg128 _mmSetPdw(int32_t w, int32_t z, int32_t y, int32_t x) {
	const int32_t tmp[4] = {w,z,y,x};
	return reg_loadu_ps(reinterpret_cast<const float*>(tmp));
}
inline reg128 _mmSetPdw(int32_t d) {
	return _mmSetPdw(d,d,d,d);
}

inline float Rcp22Bit(float s) {
	reg128 tmp = reg_load_ss(&s);
	RCP22BIT(tmp)
	float ret;
	reg_store_ss(&ret, tmp);
	return ret;
}
inline float Sqrt(float s) {
	reg_store_ss(&s, reg_sqrt_ss(reg_load_ss(&s)));
	return s;
}
inline float RSqrt(float s) {
	return Rcp22Bit(spn::Sqrt(s));
}

constexpr static uint32_t Fullbit = 0xffffffff,
						Absbit = 0x7fffffff;
constexpr float FLOAT_EPSILON = 1e-5f;		//!< 2つの値を同一とみなす誤差
// ベクトルレジスタ用の定数
class xmm_const {
	protected:
		static void Initialize();
		static void Terminate();

	#define DefXmm(name) \
		private: static reg128 _##name; \
		public: static const reg128& name() { return _##name; }
	DefXmm(tmp0001)
	DefXmm(tmp0001_i)
	DefXmm(tmp0111)
	DefXmm(tmp0111_i)
	DefXmm(tmp0000)
	DefXmm(tmp0000_i)
	DefXmm(tmp1000)
	DefXmm(tmp1000_i)
	DefXmm(tmp1111)
	DefXmm(tmp1111_i)
	DefXmm(epsilon)
	DefXmm(epsilonM)
	DefXmm(minus0)
	DefXmm(fullbit)
	DefXmm(absmask)
	#undef DefXmm

	using XmmA4 = const reg128 (&)[4];
	#define DefXmmA(name, n) \
		private: static reg128 _##name[n]; \
		public: static XmmA4 name() { return _##name; }
	DefXmmA(mask, 4)
	DefXmmA(maskN, 4)
	DefXmmA(matI, 4)
	#undef DefXmmA

	using FA44 = const float (&)[4][4];
	private: static float _cs_matI[4][4];
	public: static FA44 cs_matI() { return _cs_matI; }
};
}

#include "structure/niftycounter.hpp"
namespace spn {
DEF_NIFTY_INITIALIZER(xmm_const)
//! レジスタ要素が全てゼロか判定 (+0 or -0)
inline bool _mmIsZero(reg128 r) {
	r = reg_andnot_ps(xmm_const::minus0(), r);
	r = reg_cmpeq_ps(r, xmm_const::tmp0000());
	return reg_movemask_ps(r) == 0;
}
//! 誤差を含んだゼロ判定
inline bool _mmIsZeroEps(reg128 r) {
	reg128 xm0 = reg_cmplt_ps(r, xmm_const::epsilon()),
			xm1 = reg_cmpnle_ps(r, xmm_const::epsilonM());
	xm0 = reg_or_ps(xm0, xm1);
	return reg_movemask_ps(xm0) == 0;
}

template <int A, int B, int C, int D>
inline reg128 _makeMask() {
	reg128 zero = reg_setzero_ps();
	reg128 full = reg_andnot_ps(zero, zero);
	const int tmp2 = (((~D)&1)<<3)|(((~C)&1)<<2)|(((~B)&1)<<1)|((~A)&1);
	reg128 tmp = reg_move_ss(zero, full);
	return reg_shuffle_ps(tmp, tmp, tmp2);
}
template <class T>
constexpr T Pi = T(3.1415926535);	// std::atan(1.0f)*4
constexpr float PI = Pi<float>,
				SIN0 = 0,
				SIN30 = 1.f/2,
				SIN45 = 1.f/1.41421356f,
				SIN60 = 1.7320508f/2,
				SIN90 = 1.f,
				COS0 =  SIN90,
				COS30 = SIN60,
				COS45 = SIN45,
				COS60 = SIN30,
				COS90 = SIN0;
template <class T>
T Square(const T& t0) { return t0*t0; }
template <class T>
T Cubic(const T& t0) { return t0*t0*t0; }
// 代わりにDegree::ToRadian(ang)を使う
inline float DEGtoRAD(float ang) {
	return ang * spn::Rcp22Bit(180.f) * PI;
}
// 代わりにRadian::ToDegree(ang)を使う
inline float RADtoDEG(float ang) {
	return ang * spn::Rcp22Bit(PI) * 180.f;
}

template <class T>
struct TrivialWrapperBase : boost::serialization::traits<T,
							boost::serialization::object_serializable,
							boost::serialization::track_selectively>
{};
//! Tをtrivialなctorでラップ
template <class T>
struct TrivialWrapper : TrivialWrapperBase<TrivialWrapper<T>> {
	uint8_t _buff[sizeof(T)];

	T& operator * () { return (T&)*this; }
	const T& operator * () const { return (const T&)*this; }
	operator T& () { return *reinterpret_cast<T*>(_buff); }
	operator const T& () const { return *reinterpret_cast<const T*>(_buff); }
	T* operator -> () { return reinterpret_cast<T*>(_buff); }
	const T* operator -> () const { return reinterpret_cast<const T*>(_buff); }
	TrivialWrapper<T>& operator = (const T& t) {
		((T&)(*this)) = t;
		return *this;
	}

	friend class boost::serialization::access;
	template <class Archive>
	void serialize(Archive& ar, const unsigned int /*ver*/) {
		ar & boost::serialization::make_nvp("value", static_cast<T&>(*this));
	}

	template <class TA>
	bool operator == (const TA& t) const {
		return static_cast<const T&>(*this) == static_cast<const T&>(t);
	}
	template <class TA>
	bool operator != (const TA& t) const {
		return static_cast<const T&>(*this) != static_cast<const T&>(t);
	}
};
template <class T, int N>
struct TrivialWrapper<T[N]> : TrivialWrapperBase<TrivialWrapper<T[N]>> {
	T _buff[N];
	using AR = T (&)[N];
	using AR_C = const T (&)[N];

	AR operator * () { return _buff; }
	AR_C operator * () const { return _buff; }
	operator AR () { return _buff; }
	T& operator [] (int n) { return _buff[n]; }
	const T& operator [] (int n) const { return _buff[n]; }

	friend class boost::serialization::access;
	template <class Archive>
	void serialize(Archive& ar, const unsigned int /*ver*/) {
		ar & boost::serialization::make_nvp("value", _buff);
	}

	template <class TA>
	bool operator == (const TA& t) const {
		for(int i=0 ; i<N ; i++) {
			if(_buff[i] != t[i])
				return false;
		}
		return true;
	}
	template <class TA>
	bool operator != (const TA& t) const {
		return !(this->operator == (t));
	}
};
}

// 次元毎のロード/ストア関数を定義
// LOADPS_[ZeroFlag][AlignedFlag][Dim]
#define LOADPS_A4(ptr)		reg_load_ps(ptr)			// アラインメント済み4次元ベクトル読み込み
#define LOADPS_4(ptr)		reg_loadu_ps(ptr)
#define LOADPS_ZA4(ptr)		LOADPS_A4(ptr)				// 空き要素がゼロ初期化付き
#define LOADPS_Z4(ptr)		LOADPS_4(ptr)
#define LOADPS_IA4(ptr,n)	LOADPS_A4(ptr)				// 空きかつ対角要素のみが1、他は0
#define LOADPS_I4(ptr,n)	LOADPS_4(ptr)
#define STOREPS_A4(ptr, r)	reg_store_ps(ptr, r)		// アラインメント済み4次元ベクトル書き込み
#define STOREPS_4(ptr, r)	reg_storeu_ps(ptr, r)

#define LOADPS_A3(ptr)		LOADPS_A4(ptr)
#define LOADPS_3(ptr)		LOADPS_4(ptr)
#define LOADPS_BASE3(ptr,src,lfunc)	reg_mul_ps(src, lfunc(ptr))
#define LOADPS_ZA3(ptr)		reg_and_ps(spn::xmm_const::tmp0111_i(), reg_load_ps(ptr))
#define LOADPS_Z3(ptr)		reg_and_ps(spn::xmm_const::tmp0111_i(), reg_loadu_ps(ptr))
#define LOADPS_IA3(ptr,n)	BOOST_PP_IF(BOOST_PP_EQUAL(n,3), \
										reg_or_ps(spn::xmm_const::matI()[3], LOADPS_ZA3(ptr)), \
										LOADPS_ZA3(ptr))
#define LOADPS_I3(ptr,n)	BOOST_PP_IF(BOOST_PP_EQUAL(n,3), \
										reg_or_ps(spn::xmm_const::matI()[3], LOADPS_Z3(ptr)), \
										LOADPS_Z3(ptr))
#define STOREPS_A3(ptr, r)	{ reg_storel_pi((reg64i*)ptr, r); reg_store_ss(ptr+2, reg_shuffle_ps(r, r, _REG_SHUFFLE(2,2,2,2))); }
#define STOREPS_3(ptr, r)	STOREPS_A3(ptr,r)

#define LOADPS_A2(ptr)		LOADPS_A4(ptr)
#define LOADPS_2(ptr)		LOADPS_4(ptr)
#define LOADPS_ZA2(ptr)		reg_loadl_pi(spn::xmm_const::tmp0000(), (const reg64i*)ptr)
#define LOADPS_Z2(ptr)		LOADPS_ZA2(ptr)
#define LOADPS_IA2(ptr,n)	reg_loadl_pi(spn::xmm_const::matI()[n], (const reg64i*)ptr)
#define LOADPS_I2(ptr,n)	LOADPS_IA2(ptr,n)
#define STOREPS_A2(ptr, r)	reg_storel_pi((reg64i*)ptr,r)
#define STOREPS_2(ptr, r)	STOREPS_A2(ptr,r)

#define BOOLNIZE(b) BOOST_PP_IF(b,true,false)
#define AFLAG(a) BOOST_PP_IF(a,A,NOTHING)

//! アライン済み行列の隙間を埋める変数定義
#define GAP_MATRIX(matname, m, n, seq) GAP_MATRIX_DEF(NOTHING , matname, m, n, seq)
//! constやmutableを付ける場合の定義
#define GAP_MATRIX_DEF(prefix, matname, m, n, seq) union { prefix BOOST_PP_CAT(BOOST_PP_CAT(spn::AMat,m),n) matname; struct { BOOST_PP_SEQ_FOR_EACH_I(GAP_TFUNC_OUTER, n, seq) }; };
#define GAP_DUMMY(aux,index,amount) BOOST_PP_CAT(BOOST_PP_CAT(BOOST_PP_CAT(float dummy, __LINE__), aux), index)[amount];
//! Tupleの要素最後尾のsizeofを足し合わせる
#define COUNT_SIZE(tup)			((BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(tup), COUNT_SIZE2, tup)+3)/4)
#define COUNT_SIZE2(z,idx,data)	+sizeof(GET_LAST(BOOST_PP_TUPLE_ELEM(idx,data)))
#define GET_LAST(tup) BOOST_PP_TUPLE_ELEM(BOOST_PP_DEC(BOOST_PP_TUPLE_SIZE(tup)), tup)
//! 要素のサイズを考慮しつつ, 16byte alignedになるよう前後に適切なパディングを入れる
#define GAP_TFUNC_OUTER(z,nAr,idx,elem) \
		GAP_DUMMY(_,idx,nAr) \
		BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(elem), GAP_TFUNC_INNER, elem) \
		GAP_DUMMY(_B,idx,4-nAr-COUNT_SIZE(elem))
//! 奇数要素と偶数要素を連結して変数宣言とする
#define GAP_TFUNC_INNER(z,idx,data) BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(BOOST_PP_TUPLE_ELEM(idx,data)),2), GAP_TFUNC_INNER2, GAP_TFUNC_INNER3)(BOOST_PP_TUPLE_ELEM(idx,data))
#define GAP_TFUNC_INNER2(tup) ARGPAIR(BOOST_PP_TUPLE_ELEM(0,tup), BOOST_PP_TUPLE_ELEM(1,tup))
#define GAP_TFUNC_INNER3(tup) BOOST_PP_TUPLE_ELEM(0,tup) ARGPAIR(BOOST_PP_TUPLE_ELEM(1,tup), BOOST_PP_TUPLE_ELEM(2,tup))
#define ARGPAIR(a,b)	spn::TrivialWrapper<a> b;

//! アライン済みベクトルの隙間を埋める定義
#define GAP_VECTOR(vecname, n, seq) GAP_VECTOR_DEF(NOTHING, vecname, n, seq)
#define GAP_VECTOR_DEF(prefix, vecname, n, seq)	union { prefix BOOST_PP_CAT(spn::AVec, n) vecname; struct { float _dummy[n]; BOOST_PP_SEQ_FOR_EACH(GAP_TFUNC_VEC, NOTHING, seq) }; };
#define GAP_TFUNC_VEC(z,dummy,elem)	elem;

// ----------- ベクトルや行列のプロトタイプ定義 -----------
namespace spn {
	enum AXIS_FLAG {
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
		NUM_AXIS
	};

	template <int N, bool A>
	struct VecT;

	struct MatBase {
		//! 対角線上に数値を設定。残りはゼロ
		static struct _TagDiagonal {} TagDiagonal;
		static struct _TagIdentity {} TagIdentity;
		//! 全てを対象
		static struct _TagAll {} TagAll;
	};
	template <int M, int N, bool A>
	struct MatT;

	template <bool A>
	struct PlaneT;

	struct QuatBase {
		static struct _TagIdentity {} TagIdentity;
	};
	template <bool A>
	struct QuatT;
	template <bool A>
	struct ExpQuatT;

	//! Vector型の時はtrue_type, それ以外はfalse_type
	template <class T>
	struct IsVectorT : std::false_type {};
	template <int N, bool A>
	struct IsVectorT<spn::VecT<N,A>> : std::true_type {};
	//! Matrix型の時はtrue_type, それ以外はfalse_type
	template <class T>
	struct IsMatrixT : std::false_type {};
	template <int M, int N, bool A>
	struct IsMatrixT<spn::MatT<M,N,A>> : std::true_type {};
	//! Quaternion型の時はtrue_type, それ以外はfalse_type
	template <class T>
	struct IsQuatT : std::false_type {};
	template <bool A>
	struct IsQuatT<spn::QuatT<A>> : std::true_type {};
}
namespace std {
	template <int N, bool A>
	struct hash<spn::VecT<N,A>> {
		size_t operator()(const spn::VecT<N,A>& v) const {
			auto* src = reinterpret_cast<const uint32_t*>(&v);
			size_t tmp = 0xc000feee;
			for(int i=0 ; i<N ; i++)
				tmp ^= size_t(src[i] << (i+3)) ^ src[i];
			return tmp;
		}
	};
	template <int M, int N, bool A>
	struct hash<spn::MatT<M,N,A>> {
		size_t operator()(const spn::MatT<M,N,A>& m) const {
			size_t tmp = 0xc000feee;
			for(int i=0 ; i<M ; i++) {
				auto* src = reinterpret_cast<const uint32_t*>(m.ma[i]);
				for(int j=0 ; j<N ; j++)
					tmp ^= (src[j] << (j+3)) ^ src[j];
			}
			return tmp;
		}
	};
	template <bool A>
	struct hash<spn::QuatT<A>> {
		size_t operator()(const spn::QuatT<A>& q) const {
			using VT = spn::VecT<4,A>;
			return hash<VT>()(reinterpret_cast<const VT&>(q));
		}
	};
}

