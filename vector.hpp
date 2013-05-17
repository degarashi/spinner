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

// アラインメントや次元毎のロード/ストア関数を定義
alignas(16)
const static float xmm_tmp0001[4]={1,0,0,0},
					xmm_tmp0111[4]={1,1,1,0},
					xmm_tmp0000[4]={0,0,0,0},
					xmm_tmp0[1] = {0};

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
#define LOADPS_BASE3(ptr,src,lfunc)	_mm_mul_ps(_mm_load_ps(src), lfunc(ptr))
#define LOADPS_ZA3(ptr)		LOADPS_BASE3(ptr,xmm_tmp0111,_mm_load_ps)
#define LOADPS_Z3(ptr)		LOADPS_BASE3(ptr,xmm_tmp0111,_mm_loadu_ps)
#define LOADPS_IA3(ptr,n)	LOADPS_BASE3(ptr, xmm_matI[n], _mm_load_ps)
#define LOADPS_I3(ptr,n)	LOADPS_BASE3(ptr, xmm_matI[n], _mm_loadu_ps)
#define STOREPS_A3(ptr, r)	{ _mm_storel_pi((__m64*)ptr, r); _mm_store_ss(ptr+2, _mm_shuffle_ps(r, r, _MM_SHUFFLE(2,2,2,2))); }
#define STOREPS_3(ptr, r)	STOREPS_A3(ptr,r)

#define LOADPS_A2(ptr)		LOADPS_A4(ptr)
#define LOADPS_2(ptr)		LOADPS_A2(ptr)
#define LOADPS_ZA2(ptr)		_mm_loadl_pi(_mm_load_ps1(xmm_tmp0), (const __m64*)ptr)
#define LOADPS_Z2(ptr)		LOADPS_ZA2(ptr)
#define LOADPS_IA2(ptr,n)	_mm_loadl_pi(_mm_load_ps(xmm_matI[n]), (const __m64*)ptr)
#define LOADPS_I2(ptr,n)	LOADPS_IA2(ptr,n)
#define STOREPS_A2(ptr, r)	_mm_storel_pi((__m64*)ptr,r)
#define STOREPS_2(ptr, r)	STOREPS_A2(ptr,r)
					
#define BOOST_PP_VARIADICS 1
#include <boost/preprocessor.hpp>
#define NOTHING
#define BOOLNIZE(b) BOOST_PP_IF(b,true,false)
#define AFLAG(a) BOOST_PP_IF(a,A,NOTHING)

// ----------- ベクトル基底定義 -----------
#include "vector_base.hpp"
#include "vector_in.hpp"
