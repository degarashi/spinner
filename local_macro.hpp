/*! 予め次のマクロを定義してからインクルードする
	DIM:			次元(number)
	ALIGN:			アラインメント(0 or 1)
	DEFINE_MATRIX:	行列用なら定義 */

#define BASE_LOADPS(ZFLAG,ptr) BOOST_PP_CAT(LOADPS_, BOOST_PP_CAT(ZFLAG, BOOST_PP_CAT(AFLAG(ALIGN), DIM)))(ptr)
#ifndef DEFINE_MATRIX
	//! 自身のベクトル要素をレジスタに読み出し
	#define LOADTHIS()		BASE_LOADPS(NOTHING, (this->m))
	#define LOADTHISZ()		BASE_LOADPS(Z, (this->m))
	//! レジスタを自分のメモリ領域へ書き出し
	#define STORETHIS(r)	BOOST_PP_CAT(STOREPS_, BOOST_PP_CAT(AFLAG(ALIGN), DIM))(this->m,r)
	//! 同次元のベクトルをレジスタへ書き出し
	#define STOREPS(ptr,r)	BOOST_PP_CAT(STOREPS_, BOOST_PP_CAT(A, DIM))(ptr,r)
	#define STOREPSU(ptr,r)	BOOST_PP_CAT(STOREPS_, DIM)(ptr,r)
#else
	#define LOADTHIS(n)		BASE_LOADPS(NOTHING, (this->ma[n]))
	#define LOADTHISZ(n)	BASE_LOADPS(Z, (this->ma[n]))
	#define STORETHIS(n,r)	BOOST_PP_CAT(STOREPS_, BOOST_PP_CAT(AFLAG(ALIGN), DIM))(this->ma[n],r)
#endif

//! 同次元のベクトルをレジスタへ読み出し
#define LOADPS(ptr)		BOOST_PP_CAT(LOADPS_A, DIM)(ptr)
#define LOADPSU(ptr)	BOOST_PP_CAT(LOADPS_, DIM)(ptr)
#define LOADPSZ(ptr)	BOOST_PP_CAT(LOADPS_ZA, DIM)(ptr)
#define LOADPSZU(ptr)	BOOST_PP_CAT(LOADPS_Z, DIM)(ptr)
