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
#define DIM		BOOST_PP_ITERATION()
#define ALIGN	BOOST_PP_ITERATION_FLAGS_1()
#include "local_macro.hpp"

#define Vec		BOOST_PP_CAT(BOOST_PP_IF(ALIGN,A,NOTHING), BOOST_PP_CAT(Vec,DIM))

namespace spn {
struct Vec : VecT<DIM, BOOLNIZE(ALIGN)>, boost::equality_comparable<Vec> {
	using VecT::VecT;
	// -------------------- ctor --------------------
	Vec(const VecT& v): VecT(v) {}
	// -------------------- others --------------------
	static float _sumup(__m128 xm) {
		SUMVEC(xm)
		float ret;
		_mm_store_ss(&ret, xm);
		return ret;
	}
	// ロード関数呼び出しのコストが許容出来るケースではloadPS()を呼び、そうでないケースはオーバーロードで対処
	template <bool A>
	float dot(const VecT<DIM,A>& v) const {
		return _sumup(_mm_mul_ps(LOADTHIS(), v.loadPS()));
	}
	float average() const {
		return _sumup(LOADTHIS());
	}
	template <bool A>
	float distance(const VecT<DIM,A>& v) const {
		return std::sqrt(dist_sq(v));
	}
	template <bool A>
	float dist_sq(const VecT<DIM,A>& v) const {
		auto tv = v - *this;
		return tv.len_sq();
	}

	//! 要素ごとに最小値選択
	template <bool A>
	Vec getMin(const VecT<DIM,A>& v) const {
		return Vec(_mm_min_ps(LOADTHIS(), v.loadPS())); }
	template <bool A>
	void selectMin(const VecT<DIM,A>& v) {
		STORETHIS(_mm_min_ps(LOADTHIS(), v.loadPS())); }
	
	//! 要素ごとに最大値選択
	template <bool A>
	Vec getMax(const VecT<DIM,A>& v) const {
		return Vec(_mm_max_ps(LOADTHIS(), v.loadPS())); }
	template <bool A>
	void selectMax(const VecT<DIM,A>& v) {
		STORETHIS(_mm_max_ps(LOADTHIS(), v.loadPS())); }

	Vec operator - () const {
		return *this * -1.0f;
	}
	/*! \return 要素が全て等しい時にtrue, それ以外はfalse */
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
	/*! \return ベクトルの長さ */
	float length() const {
		return std::sqrt(len_sq());
	}
	/*! \return ベクトル長の2乗 */
	float len_sq() const {
		__m128 r0 = LOADTHISZ();
		r0 = _mm_mul_ps(r0, r0);
		SUMVEC(r0)
		
		float ret;
		_mm_store_ss(&ret, r0);
		return ret;
	}
	void saturate(float fMin, float fMax) {
		*this = saturation(fMin, fMax);
	}
	Vec saturation(float fMin, float fMax) const {
		__m128 xm = _mm_max_ps(LOADTHIS(), _mm_load1_ps(&fMin));
		return Vec(_mm_min_ps(xm, _mm_load1_ps(&fMax)));
	}
	void lerp(const UVec& v, float r) {
		*this = l_intp(v,r);
	}
	template <bool A>
	Vec l_intp(const VecT<DIM,A>& v, float r) const {
		_mm_mul_ps(_mm_load1_ps(&r), _mm_add_ps(LOADTHIS(), v.loadPS()));
	}
	
#if ALIGN==1
	//! AVec -> Vec へ暗黙変換
	operator VecT<DIM,false>& () { return *reinterpret_cast<VecT<DIM,false>*>(this); }
	operator const VecT<DIM,false>& () const { return *reinterpret_cast<const VecT<DIM,false>*>(this); }
#endif
	
#if DIM==4
	using Vec3 = VecT<3,BOOLNIZE(ALIGN)>;
	/*! \return x,y,z成分 */
	Vec3 asVec3() const {
		return Vec3(x,y,z);
	}
	/*! \return x,y,zそれぞれをwで割った値 */
	Vec3 asVec3Coord() const {
		__m128 r0 = _mm_rcp_ps(_mm_load_ps1(&w));
		return Vec3(_mm_mul_ps(LOADTHIS(), r0));
	}
#elif DIM==3
	/*! \return this X v の外積ベクトル */
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
	//! 外積計算cross()と同義
	template <bool A>
	void operator %= (const VecT<DIM,A>& v) {
		*this = cross(v);
	}
	//! 外積計算cross()と同義
	template <bool A>
	Vec operator % (const VecT<DIM,A>& v) const {
		return cross(v);
	}
	using Vec4 = VecT<4,BOOLNIZE(ALIGN)>;
	Vec4 asVec4(float w) const {
		return Vec4(x,y,z,w);
	}
	// TODO:  planeLerp
	// TODO: flip(plane)
	// TODO: *= quat
#elif DIM==2
	// ccw
	template <bool A>
	float ccw(const VecT<DIM,A>& v) const {
		return x*v.y - y*v.x;
	}
#endif

	// TODO: 行列との計算ルーチン実装(おそらく、別ファイル)
};
}
#endif
