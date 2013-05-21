//! ベクトルクラス他クラス演算ヘッダ
/*!	vector_base.hpp、matrix_base.hppの後にインクルード
	行列やクォータニオンクラスとの演算を定義 */

#if !BOOST_PP_IS_ITERATING
	#ifndef VECTOR_PROP_HEADER_
		#define VECTOR_PROP_HEADER_
		#ifdef DEFINE_MATRIX
			#undef DEFINE_MATRIX
		#endif

		#define BOOST_PP_ITERATION_PARAMS_1 (3, (2,4, "vector_prop.hpp"))
		#include BOOST_PP_ITERATE()
	#endif
#elif BOOST_PP_ITERATION_DEPTH() == 1
	#define DIM		BOOST_PP_FRAME_ITERATION(1)
	#define BOOST_PP_ITERATION_PARAMS_2 (3, (0,1, "vector_prop.hpp"))
	#include BOOST_PP_ITERATE()
#elif BOOST_PP_ITERATION_DEPTH() == 2
	#define ALIGNB	BOOLNIZE(BOOST_PP_ITERATION())
	#define VT		VecT<DIM, ALIGNB>
	#include "local_macro.hpp"
	namespace spn {
		/*	Pseudo-code:
			template <int N, bool A>
			VecT<N,ALIGNB> VT::operator * (const MatT<DIM,N,A>& m) const {
				__m128 ths = LOADTHIS(),
						accum = _mm_setzero_ps();
				for(int i=0 ; i<DIM ; i++) {
					__m128 tmp = _mm_shuffle_ps(ths, ths, _MM_SHUFFLE(i,i,i,i));
					_mm_add_ps(accum, _mm_mul_ps(tmp, LOADPS_(N)(m.ma[i])));
				}
				return VecT<N,ALIGNB>(accum);
			}
		*/
		#define LOOP_MULOP(z,n,loadf)		{__m128 tmp=_mm_shuffle_ps(ths,ths, _MM_SHUFFLE(n,n,n,n)); \
					_mm_add_ps(accum, _mm_mul_ps(tmp, loadf(mat.ma[n]))); }
		#define DEF_MULOPA(z,n,align)	template <> VecT<n,ALIGNB> VT::operator * (const MatT<DIM,n,BOOLNIZE(align)>& mat) const { \
 					__m128 ths = LOADTHIS(), \
 					accum = _mm_setzero_ps(); \
 					BOOST_PP_REPEAT_##z(n, LOOP_MULOP, BOOST_PP_CAT(LOADPS_, BOOST_PP_CAT(AFLAG(align),n))) \
 					return VecT<n,ALIGNB>(accum); }

		#define DEF_MULOP(z,align,dummy)	BOOST_PP_REPEAT_FROM_TO_##z(2,5, DEF_MULOPA, align)
		BOOST_PP_REPEAT(2, DEF_MULOP, NOTHING)
	}
#endif
