//! 行列クラス他クラス演算ヘッダ
/*!	vector_base.hpp、matrix_base.hpp, vector_prop.hppの後にインクルード
	ベクトルクラスとの演算を定義 */

#if !BOOST_PP_IS_ITERATING
	#ifndef MATRIX_PROP_H_
		#define MATRIX_PROP_H_
		
		#undef ALIGN
		#define ALIGN 0
		#define BOOST_PP_ITERATION_PARAMS_1 (3, (0, LEN_SEQ_M1, "matrix_prop.hpp"))
		#include BOOST_PP_ITERATE()
		#undef ALIGN
		#define ALIGN 1
		#define BOOST_PP_ITERATION_PARAMS_1 (3, (0, LEN_SEQ_M1, "matrix_prop.hpp"))
		#include BOOST_PP_ITERATE()
		#undef ALIGN
	#endif
#elif BOOST_PP_ITERATION_DEPTH() == 1
	#define TUP		BOOST_PP_SEQ_ELEM(BOOST_PP_ITERATION(), SEQ_MATDEF)
	#define DIM_M	BOOST_PP_TUPLE_ELEM(0, TUP)
	#define DIM_N	BOOST_PP_TUPLE_ELEM(1, TUP)
	#define ALIGNB	BOOLNIZE(ALIGN)
	#define MT		MatT<DIM_M, DIM_N, ALIGNB>
	#define DMAX 	BOOST_PP_MAX(DIM_M, DIM_N)
	#define DMIN 	BOOST_PP_MIN(DIM_M, DIM_N)
	#define DMUL	BOOST_PP_MUL(DIM_M, DIM_N)

	#define DIM		DIM_N
	#define DEFINE_MATRIX
	#include "local_macro.hpp"

	namespace spn {
		/*	Pseudo-code:
			template <>
			VecT<DIM_N,ALIGNB> operator * (const VecT<DIM_N,align>& v) const {
				auto tmpM = transposition();
				return v * tmpM;
			}
		*/
		#define DEF_MULOP(z,align,dummy)	template <> VecT<DIM_N,ALIGNB> MT::operator * (const VecT<DIM_N,BOOLNIZE(align)>& v) const { \
			auto tmpM = BOOST_PP_CAT(BOOST_PP_CAT(BOOST_PP_CAT(transposition().convert, AFLAG(ALIGN)), DIM_N), DIM_N)(); \
			return v * tmpM; }
		BOOST_PP_REPEAT(1, DEF_MULOP, NOTHING)
	}
#endif
