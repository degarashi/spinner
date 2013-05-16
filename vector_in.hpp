//! ベクトルクラス本体ヘッダ
/*! 2-4次元のベクトルをアラインメント有りと無しでそれぞれ定義 */

#if !BOOST_PP_IS_ITERATING
	#ifndef VECTOR_IN_HEADER_
		#define VECTOR_IN_HEADER_
		#include <cmath>
		#include <boost/operators.hpp>
		#define BOOST_PP_VARIADICS 1
		#include <boost/preprocessor.hpp>
		
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (2,4, "vector_in.hpp", 0))
		#include BOOST_PP_ITERATE()
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (2,4, "vector_in.hpp", 1))
		#include BOOST_PP_ITERATE()
	#endif
#else

#define ALIGN			BOOST_PP_ITERATION_FLAGS_1()
#define DIM				BOOST_PP_ITERATION()
//! 自身のベクトル要素をレジスタに読み出し
#define BASE_LOADPS(ZFLAG,ptr) BOOST_PP_CAT(LOADPS_, BOOST_PP_CAT(ZFLAG, BOOST_PP_CAT(AFLAG(ALIGN), DIM)))(ptr)
#define LOADTHIS()		BASE_LOADPS(NOTHING, (this->m))
#define LOADTHISZ()		BASE_LOADPS(Z, (this->m))
//! レジスタを自分のメモリ領域へ書き出し
#define STORETHIS(r)	BOOST_PP_CAT(STOREPS_, BOOST_PP_CAT(AFLAG(ALIGN), DIM))(this->m,r)
//! 同次元のベクトルをレジスタへ読み出し
#define LOADPS(ptr)		BOOST_PP_CAT(LOADPS_A, DIM)(ptr)
#define LOADPSU(ptr)	BOOST_PP_CAT(LOADPS_, DIM)(ptr)
#define LOADPSZ(ptr)	BOOST_PP_CAT(LOADPS_ZA, DIM)(ptr)
#define LOADPSZU(ptr)	BOOST_PP_CAT(LOADPS_Z, DIM)(ptr)
//! 同次元のベクトルをレジスタへ書き出し
#define STOREPS(ptr,r)	BOOST_PP_CAT(STOREPS_, BOOST_PP_CAT(A, DIM))(ptr,r)
#define STOREPSU(ptr,r)	BOOST_PP_CAT(STOREPS_, DIM)(ptr,r)

#define Vec		BOOST_PP_CAT(BOOST_PP_IF(ALIGN,A,NOTHING), BOOST_PP_CAT(Vec,DIM))
#define UVec	VecT<DIM,false>
#define AVec	VecT<DIM,true>

#define DEF_OP(op, func)	Vec& operator BOOST_PP_CAT(op,=) (float s) { \
		STORETHIS(func(LOADTHIS(), _mm_load_ps1(&s))); \
		return *this; } \
	Vec& operator BOOST_PP_CAT(op,=) (const AVec& v) { \
		STORETHIS(func(LOADTHIS(), LOADPS(v.m))); \
		return *this; } \
	Vec& operator BOOST_PP_CAT(op,=) (const UVec& v) { \
		STORETHIS(func(LOADTHIS(), LOADPSU(v.m))); \
		return *this; } \
	AVec&& operator op (AVec&& v) const { \
		STOREPS(v.m, func(LOADTHIS(), LOADPS(v.m))); \
		return std::forward<AVec>(v); } \
	UVec&& operator op (UVec&& v) const { \
		STOREPSU(v.m, func(LOADTHIS(), LOADPSU(v.m))); \
		return std::forward<UVec>(v); }

struct Vec : VecT<DIM, BOOLNIZE(ALIGN)>, boost::equality_comparable<Vec> {
	using VecT::VecT;
	Vec(const VecT<DIM,true>& v) {
		STORETHIS(LOADPS(v.m));
	}
	Vec(const VecT<DIM,false>& v) {
		STORETHIS(LOADPSU(v.m));
	}
	Vec& operator = (const AVec& v) {
		STORETHIS(LOADPS(v.m));
		return *this;
	}
	Vec& operator = (const UVec& v) {
		STORETHIS(LOADPSU(v.m));
		return *this;
	}
	
	// オペレータ定義
	// ベクトルとの積算や除算は同じ要素同士の演算とする
	DEF_OP(+, _mm_add_ps)
	DEF_OP(-, _mm_sub_ps)
	DEF_OP(*, _mm_mul_ps)
	DEF_OP(/, _mm_div_ps)

	// ロード関数呼び出しのコストが許容出来るケースではloadPS()を呼び、そうでないケースはオーバーロードで対処
	template <bool A>
	float dot(const VecT<DIM,A>& v) const {
		__m128 m0 = _mm_mul_ps(LOADTHIS(), v.loadPS());
		SUMVEC(m0)
		float ret;
		_mm_store_ss(&ret, m0);
		return ret;
	}

	// 最小値選択
	template <bool A>
	Vec getMin(const VecT<DIM,A>& v) const {
		return Vec(_mm_min_ps(LOADTHIS(), v.loadPS())); }
	template <bool A>
	void selectMin(const VecT<DIM,A>& v) {
		STORETHIS(_mm_min_ps(LOADTHIS(), v.loadPS())); }
	
	// 最大値選択
	template <bool A>
	Vec getMax(const VecT<DIM,A>& v) const {
		return Vec(_mm_max_ps(LOADTHIS(), v.loadPS())); }
	template <bool A>
	void selectMax(const VecT<DIM,A>& v) {
		STORETHIS(_mm_max_ps(LOADTHIS(), v.loadPS())); }

	Vec operator - () const {
		const float c_tmp = -1.0f;
		return Vec(_mm_mul_ps(LOADTHIS(), _mm_load_ps1(&c_tmp)));
	}
	template <bool A>
	bool operator == (const VecT<DIM,A>& v) const {
		__m128 r0 = _mm_cmpeq_ps(LOADTHIS(), v.loadPS());
		r0 = _mm_and_ps(r0, _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(1,0,3,2)));
		r0 = _mm_and_ps(r0, _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(0,1,2,3)));
		return _mm_cvttps_pi32(r0) != 0;
	}
	
	void normalize() {
		*this = normalization();
	}
	Vec normalization() const {
		float tmp = length();
		__m128 r0 = _mm_load_ps1(&tmp);
		r0 = _mm_div_ps(LOADTHIS(), r0);
		return Vec(r0);
	}
	//! ベクトルの長さ
	float length() const {
		return std::sqrt(len_sq());
	}
	//! ベクトル長の2乗
	float len_sq() const {
		__m128 r0 = LOADTHISZ();
		r0 = _mm_mul_ps(r0, r0);
		SUMVEC(r0)
		
		float ret;
		_mm_store_ss(&ret, r0);
		return ret;
	}
	
#if DIM==4
	using Vec3 = VecT<3,BOOLNIZE(ALIGN)>;
	Vec3 asVec3() const {
		return Vec3(x,y,z);
	}
	Vec3 asVec3Coord() const {
		__m128 r0 = _mm_rcp_ps(_mm_load_ps1(&w));
		return Vec3(_mm_mul_ps(LOADTHIS(), r0));
	}
#elif DIM==3
	template <bool A>
	Vec cross(const VecT<DIM,A>& v) const {
		__m128 r0 = LOADTHIS(),
				r1 = v.loadPS();
		// r0[y,z,x]
		__m128 m0 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(0,0,2,1)),
		// r0[z,x,y]
				m1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(0,1,0,2)),
		// r1[z,x,y]
				m2 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0,1,0,2)),
		// r1[y,z,x]
				m3 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0,0,2,1));
		r0 = _mm_mul_ps(m0,m2);
		r1 = _mm_mul_ps(m1,m3);
		return Vec(_mm_sub_ps(r0, r1));
	}
	template <bool A>
	void operator %= (const VecT<DIM,A>& v) {
		*this = cross(v);
	}
	template <bool A>
	Vec operator % (const VecT<DIM,A>& v) const {
		return cross(v);
	}
	using Vec4 = VecT<4,BOOLNIZE(ALIGN)>;
	Vec4 asVec4(float w) const {
		return Vec4(x,y,z,w);
	}
	// planeLerp
	// flip(plane)
	// *= quat
#elif DIM==2
	// ccw
	template <bool A>
	float ccw(const VecT<DIM,A>& v) const {
		return x*v.y - y*v.x;
	}
#endif

};

#undef DIM
#undef ALIGN
#undef DEF_OP
#undef BASE_LOADPS
#undef LOADTHIS
#undef LOADTHISZ
#undef STORETHIS
#undef LOADPS
#undef LOADPSU
#undef LOADPSZ
#undef LOADPSZU
#undef STOREPS
#undef STOREPSU
#undef Vec
#undef UVec
#undef AVec

#endif
