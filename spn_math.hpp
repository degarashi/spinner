#pragma once
#include "macro.hpp"

#ifndef SSE_LEVEL
	#define SSE_LEVEL 1
#endif

#if SSE_LEVEL >= 3
	#include <pmmintrin.h>
#elif SSE_LEVEL == 2
	#include <emmintrin.h>
#else
	#include <xmmintrin.h>
#endif

#if SSE_LEVEL <= 2
	#define SUMVEC(r)	{ __m128 tmp = _mm_shuffle_ps(r, r, _MM_SHUFFLE(0,1,2,3)); \
			tmp = _mm_add_ps(r, tmp); \
			r = _mm_shuffle_ps(tmp,tmp, _MM_SHUFFLE(1,0,0,1)); \
			r = _mm_add_ps(r, tmp); }
#else
	// rの各要素を足し合わせる
	#define SUMVEC(r)	{ r = _mm_hadd_ps(r,r); \
			r = _mm_hadd_ps(r,r); }
#endif

#define RCP22BIT(r) { __m128 tmprcp = _mm_rcp_ps(r); \
	r = _mm_mul_ps(r, _mm_mul_ps(tmprcp, tmprcp)); \
	tmprcp = _mm_add_ps(tmprcp,tmprcp); \
	r = _mm_sub_ps(tmprcp, r); }

#include <cmath>
#include <cstdint>
namespace spn {
//! ニュートン法で逆数を計算
inline __m128 _mmRcp22Bit(__m128 r) {
	__m128 tmp = _mm_rcp_ps(r);
	r = _mm_mul_ps(r, _mm_mul_ps(tmp,tmp));
	tmp = _mm_add_ps(tmp,tmp);
	return _mm_sub_ps(tmp, r);
}
//! ニュートン法で逆数を計算して積算 = _mm_div_psよりはマシな除算
inline __m128 _mmDivPs(__m128 r0, __m128 r1) {
	return _mm_mul_ps(r0, _mmRcp22Bit(r1));
}
inline __m128 _mmAbsPs(__m128 r) {
	const static __m128 signMask = _mm_set1_ps(-0.0f);
	return _mm_andnot_ps(signMask, r);
}
inline __m128 _mmSetPs(float w, float z, float y, float x) {
	const float tmp[4]{w,z,y,x};
	return _mm_loadu_ps(tmp);
}
inline __m128 _mmSetPs(float s) {
	return _mm_load1_ps(&s);
}
inline float _sseRcp22Bit(float s) {
	__m128 tmp = _mm_load_ss(&s);
	RCP22BIT(tmp)
	float ret;
	_mm_store_ss(&ret, tmp);
	return ret;
}
inline float _sseSqrt(float s) {
	_mm_store_ss(&s, _mm_sqrt_ss(_mm_load_ss(&s)));
	return s;
}
inline float _sseRSqrt(float s) {
	return _sseRcp22Bit(_sseSqrt(s));
}

constexpr const static float FLOAT_EPSILON = 1e-5f;		//!< 2つの値を同一とみなす誤差
// SSEレジスタ用の定数
const static __m128 xmm_tmp0001(_mmSetPs(1,0,0,0)),
					xmm_tmp0111(_mmSetPs(1,1,1,0)),
					xmm_tmp0000(_mmSetPs(0,0,0,0)),
					xmm_tmp1000(_mmSetPs(0,0,0,1)),
					xmm_tmp1111(_mmSetPs(1)),
					xmm_epsilon(_mmSetPs(FLOAT_EPSILON)),
					xmm_epsilonM(_mmSetPs(-FLOAT_EPSILON)),
					xmm_minus0(_mmSetPs(-0.f, -0.f, -0.f, -0.f));

//! レジスタ要素が全てゼロか判定 (+0 or -0)
inline bool _mmIsZero(__m128 r) {
	r = _mm_andnot_ps(xmm_minus0, r);
	r = _mm_cmpeq_ps(r, xmm_tmp0000);
	return _mm_movemask_ps(r) == 0;
}
//! 誤差を含んだゼロ判定
inline bool _mmIsZeroEps(__m128 r) {
	__m128 xm0 = _mm_cmplt_ps(r, xmm_epsilon),
			xm1 = _mm_cmpnle_ps(r, xmm_epsilonM);
	xm0 = _mm_or_ps(xm0, xm1);
	return _mm_movemask_ps(xm0) == 0;
}

template <int A, int B, int C, int D>
inline __m128 _makeMask() {
	__m128 zero = _mm_setzero_ps();
	__m128 full = _mm_andnot_ps(zero, zero);
	const int tmp2 = (((~D)&1)<<3)|(((~C)&1)<<2)|(((~B)&1)<<1)|((~A)&1);
	__m128 tmp = _mm_move_ss(zero, full);
	return _mm_shuffle_ps(tmp, tmp, tmp2);
}
const static __m128 xmm_mask[4] = {_makeMask<1,0,0,0>(),
									_makeMask<1,1,0,0>(),
									_makeMask<1,1,1,0>(),
									_makeMask<1,1,1,1>()},
					xmm_maskN[4] = {_makeMask<0,1,1,1>(),
									_makeMask<0,0,1,1>(),
									_makeMask<0,0,0,1>(),
									_makeMask<0,0,0,0>()};
const static float cs_matI[4][4] = {
	{1,0,0,0},
	{0,1,0,0},
	{0,0,1,0},
	{0,0,0,1}
};
const static __m128 xmm_matI[4] = {
	_mmSetPs(1,0,0,0),
	_mmSetPs(0,1,0,0),
	_mmSetPs(0,0,1,0),
	_mmSetPs(0,0,0,1)
};
constexpr float PI = 3.1415926535f,		// std::atan(1.0f)*4;
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
inline float DEGtoRAD(float ang) {
	return ang*_sseRcp22Bit(180.f) * PI;
}
inline float RADtoDEG(float ang) {
	return ang*_sseRcp22Bit(PI) * 180.f;
}
}
// 次元毎のロード/ストア関数を定義
// LOADPS_[ZeroFlag][AlignedFlag][Dim]
#define LOADPS_A4(ptr)		_mm_load_ps(ptr)			// アラインメント済み4次元ベクトル読み込み
#define LOADPS_4(ptr)		_mm_loadu_ps(ptr)
#define LOADPS_ZA4(ptr)		LOADPS_A4(ptr)				// 空き要素がゼロ初期化付き
#define LOADPS_Z4(ptr)		LOADPS_4(ptr)
#define LOADPS_IA4(ptr,n)	LOADPS_A4(ptr)				// 空きかつ対角要素のみが1、他は0
#define LOADPS_I4(ptr,n)	LOADPS_4(ptr)
#define STOREPS_A4(ptr, r)	_mm_store_ps(ptr, r)		// アラインメント済み4次元ベクトル書き込み
#define STOREPS_4(ptr, r)	_mm_storeu_ps(ptr, r)

#define LOADPS_A3(ptr)		LOADPS_A4(ptr)
#define LOADPS_3(ptr)		LOADPS_4(ptr)
#define LOADPS_BASE3(ptr,src,lfunc)	_mm_mul_ps(src, lfunc(ptr))
#define LOADPS_ZA3(ptr)		_mm_mul_ps(spn::xmm_tmp0111, _mm_load_ps(ptr))
#define LOADPS_Z3(ptr)		_mm_mul_ps(spn::xmm_tmp0111, _mm_loadu_ps(ptr))
#define LOADPS_IA3(ptr,n)	BOOST_PP_IF(BOOST_PP_EQUAL(n,3), \
										_mm_or_ps(spn::xmm_matI[3], LOADPS_ZA3(ptr)), \
										LOADPS_ZA3(ptr))
#define LOADPS_I3(ptr,n)	BOOST_PP_IF(BOOST_PP_EQUAL(n,3), \
										_mm_or_ps(spn::xmm_matI[3], LOADPS_Z3(ptr)), \
										LOADPS_Z3(ptr))
#define STOREPS_A3(ptr, r)	{ _mm_storel_pi((__m64*)ptr, r); _mm_store_ss(ptr+2, _mm_shuffle_ps(r, r, _MM_SHUFFLE(2,2,2,2))); }
#define STOREPS_3(ptr, r)	STOREPS_A3(ptr,r)

#define LOADPS_A2(ptr)		LOADPS_A4(ptr)
#define LOADPS_2(ptr)		LOADPS_4(ptr)
#define LOADPS_ZA2(ptr)		_mm_loadl_pi(spn::xmm_tmp0000, (const __m64*)ptr)
#define LOADPS_Z2(ptr)		LOADPS_ZA2(ptr)
#define LOADPS_IA2(ptr,n)	_mm_loadl_pi(spn::xmm_matI[n], (const __m64*)ptr)
#define LOADPS_I2(ptr,n)	LOADPS_IA2(ptr,n)
#define STOREPS_A2(ptr, r)	_mm_storel_pi((__m64*)ptr,r)
#define STOREPS_2(ptr, r)	STOREPS_A2(ptr,r)

#define BOOLNIZE(b) BOOST_PP_IF(b,true,false)
#define AFLAG(a) BOOST_PP_IF(a,A,NOTHING)

//! アライン済み行列の隙間を埋める変数定義
#define GAP_MATRIX(matname, m, n, seq) GAP_MATRIX_DEF(NOTHING , matname, m, n, seq)
//! constやmutableを付ける場合の定義
#define GAP_MATRIX_DEF(prefix, matname, m, n, seq) union { prefix BOOST_PP_CAT(BOOST_PP_CAT(spn::AMat,m),n) matname; struct { BOOST_PP_SEQ_FOR_EACH_I(GAP_TFUNC_OUTER, n, seq) }; };
#define GAP_DUMMY(aux,index,amount) BOOST_PP_CAT(BOOST_PP_CAT(BOOST_PP_CAT(float dummy, __LINE__), aux), index)[amount];
//! Tupleの奇数要素のsizeofを足し合わせる
#define COUNT_SIZE(tup)			((BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(tup), COUNT_SIZE2, tup)+3)/4)
#define COUNT_SIZE2(z,idx,data)	BOOST_PP_IF(BOOST_PP_MOD(idx,2), +sizeof(BOOST_PP_TUPLE_ELEM(idx,data)), NOTHING)
//! 要素のサイズを考慮しつつ, 16byte alignedになるよう前後に適切なパディングを入れる
#define GAP_TFUNC_OUTER(z,nAr,idx,elem) \
		GAP_DUMMY(_,idx,4-nAr) \
		BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(elem), GAP_TFUNC_INNER, elem) \
		GAP_DUMMY(_B,idx,4-nAr-COUNT_SIZE(elem))
//! 奇数要素と偶数要素を連結して変数宣言とする
#define GAP_TFUNC_INNER(z,idx,data) BOOST_PP_IF(BOOST_PP_MOD(idx,2), NOTHING_ARG, ARGPAIR)(BOOST_PP_TUPLE_ELEM(idx,data), BOOST_PP_TUPLE_ELEM(BOOST_PP_INC(idx),data))
#define ARGPAIR(a,b)	a b;

//! アライン済みベクトルの隙間を埋める定義
#define GAP_VECTOR(vecname, n, seq) GAP_VECTOR_DEF(NOTHING, vecname, n, seq)
#define GAP_VECTOR_DEF(prefix, vecname, n, seq)	union { prefix BOOST_PP_CAT(spn::AVec, n) vecname; struct { float _dummy[4-n]; BOOST_PP_SEQ_FOR_EACH(GAP_TFUNC_VEC, NOTHING, seq) }; };
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
}
