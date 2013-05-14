//! ベクトルクラスヘッダ
/*!	事前にDIM, ALIGNを定義
	DIM = 2-4
	ALIGN = (nothing) or 'A' */

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
		float ret;
#ifdef USE_SSE3
		m0 = _mm_hadd_ps(m0,m0);
		_mm_store_ss(&ret, _mm_hadd_ps(m0,m0));
#else
		__m128 m1 = _mm_shuffle_ps(m0, m0, _MM_SHUFFLE(0,1,0,1));
		m1 = _mm_add_ps(m0,m1);
		m0 = _mm_shuffle_ps(m1,m1, _MM_SHUFFLE(1,1,1,1));
		_mm_store_ss(&ret, _mm_add_ss(m0,m1));
#endif
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
		__m128 m0 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(0,1,2,0)),
		// r0[z,x,y]
				m1 = _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(0,2,0,1)),
		// r1[z,x,y]
				m2 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0,2,0,1)),
		// r1[y,z,x]
				m3 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(0,1,2,0));
		r0 = _mm_mul_ps(m0,m1);
		r1 = _mm_mul_ps(m2,m3);
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
