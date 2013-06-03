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
#else
	#define LOADTHIS(n)			BASE_LOADPS(NOTHING, (this->ma[n]))
	#define LOADTHISZ(n)		BASE_LOADPS(Z, (this->ma[n]))
	#define LOADTHISI(n)		BOOST_PP_CAT(LOADPS_I, BOOST_PP_CAT(AFLAG(ALIGN), DIM))(this->ma[n], n)
	#define STORETHIS(n,r)		BOOST_PP_CAT(STOREPS_, BOOST_PP_CAT(AFLAG(ALIGN), DIM))(this->ma[n],r)
#endif

//! 同次元のベクトルをレジスタへ書き出し
#define STOREPS(ptr,r)	BOOST_PP_CAT(STOREPS_, BOOST_PP_CAT(A, DIM))(ptr,r)
#define STOREPSU(ptr,r)	BOOST_PP_CAT(STOREPS_, DIM)(ptr,r)
//! 自身と同じアラインメント指定の要素を読み出し
#define LOADTHISPS(ptr)		BASE_LOADPS(NOTHING, ptr)
//! 同じアラインメント指定の要素を書き出し
#define STORETHISPS(ptr, r)	BOOST_PP_CAT(STOREPS_, BOOST_PP_CAT(AFLAG(ALIGN), DIM))(ptr, r)

//! 同次元のベクトルをレジスタへ読み出し
#define LOADPS(ptr)		BOOST_PP_CAT(LOADPS_A, DIM)(ptr)
#define LOADPSU(ptr)	BOOST_PP_CAT(LOADPS_, DIM)(ptr)
#define LOADPSZ(ptr)	BOOST_PP_CAT(LOADPS_ZA, DIM)(ptr)
#define LOADPSZU(ptr)	BOOST_PP_CAT(LOADPS_Z, DIM)(ptr)
