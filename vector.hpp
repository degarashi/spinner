#pragma once
#ifdef USE_SSE3
	#include <pmmintrin.h>
#else
	#include <xmmintrin.h>
#endif

#define BOOST_PP_VARIADICS 1
#include <boost/preprocessor.hpp>

// アラインメントや次元毎のロード/ストア関数を定義
alignas(16)
const static float xmm_tmp1000[4]={1.0f, 0, 0, 0},
					xmm_tmp0111[4]={0,1,1,1},
					xmm_tmp0000[4]={0,0,0,0},
					xmm_tmp0[1] = {0};

// LOADPS_[ZeroFlag][AlignedFlag][Dim]
#define LOADPS_A4(ptr)		_mm_load_ps(ptr)
#define LOADPS_4(ptr)		_mm_loadu_ps(ptr)
#define LOADPS_ZA4(ptr)		LOADPS_A4(ptr)
#define LOADPS_Z4(ptr)		LOADPS_4(ptr)
#define STOREPS_A4(ptr, r)	_mm_store_ps(ptr, r)
#define STOREPS_4(ptr, r)	_mm_storeu_ps(ptr, r)

#define LOADPS_A3(ptr)		LOADPS_A4(ptr)
#define LOADPS_3(ptr)		LOADPS_4(ptr)
#define LOADPS_ZA3(ptr)		_mm_mul_ps(_mm_load_ps(xmm_tmp0111), _mm_load_ps(ptr))
#define LOADPS_Z3(ptr)		_mm_mul_ps(_mm_load_ps(xmm_tmp0111), _mm_loadu_ps(ptr))
#define STOREPS_A3(ptr, r)	{ _mm_storel_pi((__m64*)ptr, r); _mm_store_ss(ptr+2, _mm_shuffle_ps(r, r, _MM_SHUFFLE(2,2,2,2))); }
#define STOREPS_3(ptr, r)	STOREPS_A3(ptr,r)

#define LOADPS_A2(ptr)		LOADPS_A4(ptr)
#define LOADPS_2(ptr)		LOADPS_A2(ptr)
#define LOADPS_ZA2(ptr)		_mm_loadl_pi(_mm_load_ps1(xmm_tmp0), (const __m64*)ptr)
#define LOADPS_Z2(ptr)		LOADPS_ZA2(ptr)
#define STOREPS_A2(ptr, r)	_mm_storel_pi((__m64*)ptr,r)
#define STOREPS_2(ptr, r)	STOREPS_A2(ptr,r)

// ----------- ベクトル基底定義 -----------
#define SEQ_VECELEM (x)(y)(z)(w)
#define NOTHING
#define BOOLNIZE(b) BOOST_PP_IF(b,true,false)
#define AFLAG(a) BOOST_PP_IF(a,A,NOTHING)
#define DEF_REGSET(align, n)	VecT(__m128 r){ BOOST_PP_CAT(BOOST_PP_CAT(STOREPS_, AFLAG(align)), n)(m, r); }
#define DEF_VECBASE(z,n,align)	template <> struct BOOST_PP_IF(align, alignas(16), NOTHING) \
	VecT<n, BOOLNIZE(align)> { \
	union { \
		struct {float BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, n)); }; \
		float m[n]; \
	}; \
	VecT() = default; \
	DEF_REGSET(align,n) \
	VecT(ENUM_ARGPAIR(BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, n))) { \
		BOOST_PP_SEQ_FOR_EACH(DEF_ARGSET, NOTHING, BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, n)) } \
	__m128 loadPS() const { return BOOST_PP_IF(align, _mm_load_ps, _mm_loadu_ps)(m); } };

#define DEF_VECS(align) BOOST_PP_REPEAT_FROM_TO(2,5,DEF_VECBASE,align)
	
#define DEF_ARGPAIR(index,data,elem)	(float BOOST_PP_CAT(f, elem))
#define ENUM_ARGPAIR(subseq) BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(DEF_ARGPAIR, NOTHING, subseq))
#define DEF_ARGSET(index,data,elem)		elem = BOOST_PP_CAT(f, elem);

template <int N, bool A>
struct VecT;

// アラインメント無し
DEF_VECS(0)
// 16byteアラインメント有り
DEF_VECS(1)

#include <boost/operators.hpp>

#define ALIGN 1
#define DIM 4
#include "vector_in.hpp"
#undef DIM
#define DIM 3
#include "vector_in.hpp"
#undef DIM
#define DIM 2
#include "vector_in.hpp"

#undef ALIGN
#define ALIGN 0
#undef DIM
#define DIM 4
#include "vector_in.hpp"
#undef DIM
#define DIM 3
#include "vector_in.hpp"
#undef DIM
#define DIM 2
#include "vector_in.hpp"
