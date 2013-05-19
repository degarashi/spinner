//! 行列クラス本体ヘッダ
/*! */
// [0,1,2]
// [3,4,5]
// matrix_in.hpp:	他の行列と計算するメソッドを定義

#define DIM		DIM_N
#define DEFINE_MATRIX
#include "local_macro.hpp"
#define Mat		BOOST_PP_CAT(BOOST_PP_IF(ALIGN,A,NOTHING), BOOST_PP_CAT(BOOST_PP_CAT(Mat,DIM_M),DIM_N))
