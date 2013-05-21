#pragma once
#ifdef USE_SSE3
	#include <pmmintrin.h>
	// rの各要素を足し合わせる
	#define SUMVEC(r)	{ r = _mm_hadd_ps(r,r); \
			r = _mm_hadd_ps(r,r); }
#else
	#include <xmmintrin.h>
	#define SUMVEC(r)	{ __m128 tmp = _mm_shuffle_ps(r, r, _MM_SHUFFLE(0,1,2,3)); \
			tmp = _mm_add_ps(r, tmp); \
			r = _mm_shuffle_ps(tmp,tmp, _MM_SHUFFLE(1,0,0,1)); \
			r = _mm_add_ps(r, tmp); }
#endif
#define RCP22BIT(r) { __m128 tmprcp = _mm_rcp_ps(r); \
	r = _mm_mul_ps(r, _mm_mul_ps(tmprcp, tmprcp)); \
	tmprcp = _mm_add_ps(tmprcp,tmprcp); \
	r = _mm_sub_ps(tmprcp, r); }

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

constexpr const static float FLOAT_EPSILON = 1e-5f;		//!< 2つの値を同一とみなす誤差
// SSEレジスタ用の定数
const static __m128 xmm_tmp0001(_mmSetPs(1,0,0,0)),
					xmm_tmp0111(_mmSetPs(1,1,1,0)),
					xmm_tmp0000(_mmSetPs(0,0,0,0)),
					xmm_tmp1000(_mmSetPs(0,0,0,1)),
					xmm_tmp1111(_mmSetPs(1)),
					xmm_epsilon(_mmSetPs(FLOAT_EPSILON)),
					xmm_epsilonM(_mmSetPs(-FLOAT_EPSILON));

template <int A, int B, int C, int D>
inline __m128 _makeMask() {
	__m128 zero = _mm_setzero_ps();
	__m128 full = _mm_andnot_ps(zero, zero);
	const int tmp2 = ((~D&1)<<3)|((~C&1)<<2)|((~B&1)<<1)|(~A&1);
	__m128 tmp = _mm_move_ss(zero, full);
	return _mm_shuffle_ps(tmp, tmp, tmp2);
}
const static __m128 xmm_mask[4] = {_makeMask<1,0,0,0>(),
									_makeMask<1,1,0,0>(),
									_makeMask<1,1,1,0>(),
									_makeMask<1,1,1,1>()};
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
#define LOADPS_ZA3(ptr)		_mm_mul_ps(xmm_tmp0111, _mm_load_ps(ptr))
#define LOADPS_Z3(ptr)		_mm_mul_ps(xmm_tmp0111, _mm_loadu_ps(ptr))
#define LOADPS_IA3(ptr,n)	BOOST_PP_IF(BOOST_PP_EQUAL(n,3), \
										_mm_or_ps(xmm_mask[3], LOADPS_A3(ptr)), \
										LOADPS_ZA3(ptr))
#define LOADPS_I3(ptr,n)	BOOST_PP_IF(BOOST_PP_EQUAL(n,3), \
										_mm_or_ps(xmm_mask[3], LOADPS_3(ptr)), \
										LOADPS_Z3(ptr))
#define STOREPS_A3(ptr, r)	{ _mm_storel_pi((__m64*)ptr, r); _mm_store_ss(ptr+2, _mm_shuffle_ps(r, r, _MM_SHUFFLE(2,2,2,2))); }
#define STOREPS_3(ptr, r)	STOREPS_A3(ptr,r)

#define LOADPS_A2(ptr)		LOADPS_A4(ptr)
#define LOADPS_2(ptr)		LOADPS_A2(ptr)
#define LOADPS_ZA2(ptr)		_mm_loadl_pi(xmm_tmp0000, (const __m64*)ptr)
#define LOADPS_Z2(ptr)		LOADPS_ZA2(ptr)
#define LOADPS_IA2(ptr,n)	_mm_loadl_pi(xmm_matI[n], (const __m64*)ptr)
#define LOADPS_I2(ptr,n)	LOADPS_IA2(ptr,n)
#define STOREPS_A2(ptr, r)	_mm_storel_pi((__m64*)ptr,r)
#define STOREPS_2(ptr, r)	STOREPS_A2(ptr,r)
					
#define BOOST_PP_VARIADICS 1
#include <boost/preprocessor.hpp>
#define NOTHING
#define BOOLNIZE(b) BOOST_PP_IF(b,true,false)
#define AFLAG(a) BOOST_PP_IF(a,A,NOTHING)

// ----------- ベクトルや行列の定義 -----------
namespace spn {
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
}
#include "vector_base.hpp"
#include "matrix_base.hpp"
#include "vector_prop.hpp"
#include "matrix_prop.hpp"
