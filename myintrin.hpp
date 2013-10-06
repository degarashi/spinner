#pragma once

#ifdef USE_SSE
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

	#define reg128			__m128
	#define reg_add_ps		_mm_add_ps
	#define reg_sub_ps		_mm_sub_ps
	#define reg_mul_ps		_mm_mul_ps
	#define reg_load1_ps	_mm_load1_ps
	#define reg_loadu_ps	_mm_loadu_ps
	#define reg_load_ss		_mm_load_ss
	#define reg_move_ss		_mm_move_ss
	#define reg_store_ss	_mm_store_ss
	#define reg_store_ps	_mm_store_ps
	#define reg_storeu_ps	_mm_storeu_ps
	#define reg_setzero_ps	_mm_setzero_ps
	#define reg_or_ps		_mm_or_ps
	#define reg_and_ps		_mm_and_ps
	#define reg_xor_ps		_mm_xor_ps
	#define reg_setr_ps		_mm_setr_ps
	#define reg_movemask_ps	_mm_movemask_ps
	#define reg_max_ps		_mm_max_ps
	#define reg_min_ps		_mm_min_ps
	#define reg_unpacklo_ps	_mm_unpacklo_ps
	#define reg_unpackhi_ps	_mm_unpackhi_ps
	#define reg_shuffle_ps	_mm_shuffle_ps
	#define reg_setr_ps		_mm_setr_ps
	#define _REG_SHUFFLE	_MM_SHUFFLE
	#define reg_rcp_ps		_mm_rcp_ps
	#define reg_andnot_ps	_mm_andnot_ps
	#define reg_storel_pi	_mm_storel_pi
	#define reg_loadl_pi	_mm_loadl_pi
	#define reg_cvtepi32_ps	_mm_cvtepi32_ps
	#define reg_packs_epi32	_mm_packs_epi32
	#define reg_packus_epi16	_mm_packus_epi16
	#define reg_set1_ps		_mm_set1_ps
	#define reg_sqrt_ps		_mm_sqrt_ps
	#define reg_sqrt_ss		_mm_sqrt_ss
	#define reg_cmpeq_ps	_mm_cmpeq_ps
	#define reg_cmplt_ps	_mm_cmplt_ps
	#define reg_cmpnle_ps	_mm_cmpnle_ps
#elif defined(USE_NEON)
	void _Dummy__() {
		static_assert(false, "not implemented yet");
	}
#else
	void _Dummy__() {
		static_assert(false, "not implemented yet");
	}
#endif