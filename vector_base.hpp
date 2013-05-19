//! ベクトルクラス基底ヘッダ
/*! 2-4次元のベクトルをアラインメント有りと無しでそれぞれ定義
	初期化や要素毎のアクセスなど基本的なメソッド */

#if !BOOST_PP_IS_ITERATING
	#ifndef VECTOR_BASE_HEADER_
		#define VECTOR_BASE_HEADER_
		#define BOOST_PP_VARIADICS 1
		#include <boost/preprocessor.hpp>
		#include <boost/operators.hpp>
		#include <cmath>
		#define DEF_ARGPAIR(index,data,elem)	(float BOOST_PP_CAT(f, elem))
		#define ENUM_ARGPAIR(subseq) BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(DEF_ARGPAIR, NOTHING, subseq))
		#define DEF_ARGSET(index,data,elem)		elem = BOOST_PP_CAT(f, elem);
		#define SEQ_VECELEM (x)(y)(z)(w)
		namespace spn {
			template <int N, bool A>
			struct VecT;
		}
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (2,4, "vector_base.hpp", 0))
		#include BOOST_PP_ITERATE()
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (2,4, "vector_base.hpp", 1))
		#include BOOST_PP_ITERATE()
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (2,4, "vector_base.hpp", 2))
		#include BOOST_PP_ITERATE()
	#endif
#elif BOOST_PP_ITERATION_DEPTH() == 1
	// アラインメントイテレーション
	#define DIM		BOOST_PP_FRAME_ITERATION(1)
	#define BOOST_PP_ITERATION_PARAMS_2 (4, (0,1, "vector_base.hpp", BOOST_PP_FRAME_FLAGS(1)))
	#include BOOST_PP_ITERATE()
#else
	#define ALIGN	BOOST_PP_ITERATION()
	#define Vec		BOOST_PP_IF(ALIGN, alignas(16), NOTHING) VecT<DIM, BOOLNIZE(ALIGN)>
	#define VT		VecT<DIM, BOOLNIZE(ALIGN)>
	#include "local_macro.hpp"
	#if BOOST_PP_ITERATION_FLAGS() == 0
		// クラスのコンストラクタとメソッドのプロトタイプだけ定義
		namespace spn {
			template <>
			struct Vec : boost::equality_comparable<Vec> {
				enum { width = DIM };
				using AVec = VecT<DIM,true>;
				using UVec = VecT<DIM,false>;
				union {
					struct {
						float BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, DIM));
					};
					float m[DIM];
				};
				// -------------------- ctor --------------------
				VecT() = default;
				explicit VecT(__m128 r){
					BOOST_PP_CAT(BOOST_PP_CAT(STOREPS_, AFLAG(ALIGN)), DIM)(m, r);
				}
				VecT(ENUM_ARGPAIR(BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, DIM))) {
					BOOST_PP_SEQ_FOR_EACH(DEF_ARGSET, NOTHING, BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, DIM))
				}
				__m128 loadPS() const {
					return BOOST_PP_IF(ALIGN, _mm_load_ps, _mm_loadu_ps)(m);
				}
				//! アラインメント済ベクトルで初期化
				VecT(const AVec& v);
				//! アラインメント無しベクトルで初期化
				VecT(const UVec& v);
				VecT& operator = (const AVec& v);
				VecT& operator = (const UVec& v);
				VecT& operator = (__m128 r) {
					STOREPS(m, r);
					return *this;
				}
				
				// -------------------- operators --------------------
				// ベクトルとの積算や除算は同じ要素同士の演算とする
				#define DEF_PRE(op, func)	VecT& operator BOOST_PP_CAT(op,=) (float s); \
					VecT operator op (float s) const; \
					VecT& operator BOOST_PP_CAT(op,=) (const AVec& v); \
					VecT& operator BOOST_PP_CAT(op,=) (const UVec& v); \
					AVec operator op (const AVec& v) const; \
					UVec operator op (const UVec& v) const; \
					AVec&& operator op (AVec&& v) const; \
					UVec&& operator op (UVec&& v) const;
				DEF_PRE(+, _mm_add_ps)
				DEF_PRE(-, _mm_sub_ps)
				DEF_PRE(*, _mm_mul_ps)
				DEF_PRE(/, _mmDivPs)
				
				// -------------------- others --------------------
				static float _sumup(__m128 xm) {
					SUMVEC(xm)
					float ret;
					_mm_store_ss(&ret, xm);
					return ret;
				}
				// ロード関数呼び出しのコストが許容出来るケースではloadPS()を呼び、そうでないケースはオーバーロードで対処
				template <bool A>
				float dot(const VecT<DIM,A>& v) const;
				float average() const;
				template <bool A>
				float distance(const VecT<DIM,A>& v) const;
				template <bool A>
				float dist_sq(const VecT<DIM,A>& v) const;
				
				//! 要素ごとに最小値選択
				template <bool A>
				Vec getMin(const VecT<DIM,A>& v) const;
				template <bool A>
				void selectMin(const VecT<DIM,A>& v);
				
				//! 要素ごとに最大値選択
				template <bool A>
				Vec getMax(const VecT<DIM,A>& v) const;
				template <bool A>
				void selectMax(const VecT<DIM,A>& v);
				
				Vec operator - () const;
				
				/*! \return 要素が全て等しい時にtrue, それ以外はfalse */
				template <bool A>
				bool operator == (const VecT<DIM,A>& v) const;
				
				void normalize();
				Vec normalization() const;
				/*! \return ベクトルの長さ */
				float length() const;
				/*! \return ベクトル長の2乗 */
				float len_sq() const;
				
				void saturate(float fMin, float fMax);
				Vec saturation(float fMin, float fMax) const;
				void lerp(const UVec& v, float r);
				template <bool A>
				Vec l_intp(const VecT<DIM,A>& v, float r) const;
				
				#if ALIGN==1
					//! AVec -> Vec へ暗黙変換
					operator VecT<DIM,false>& ();
					operator const VecT<DIM,false>& () const;
				#endif
				#if DIM==4
					using Vec3 = VecT<3,BOOLNIZE(ALIGN)>;
					/*! \return x,y,z成分 */
					Vec3 asVec3() const;
					/*! \return x,y,zそれぞれをwで割った値 */
					Vec3 asVec3Coord() const;
				#elif DIM==3
					/*! \return this X v の外積ベクトル */
					template <bool A>
					Vec cross(const VecT<DIM,A>& v) const;
					//! 外積計算cross()と同義
					template <bool A>
					void operator %= (const VecT<DIM,A>& v);
					//! 外積計算cross()と同義
					template <bool A>
					Vec operator % (const VecT<DIM,A>& v) const;
					using Vec4 = VecT<4,BOOLNIZE(ALIGN)>;
					Vec4 asVec4(float w) const;
					// TODO:  planeLerp
					// TODO: flip(plane)
					// TODO: *= quat
				#elif DIM==2
					// ccw
					template <bool A>
					float ccw(const VecT<DIM,A>& v) const;
				#endif
				// TODO: 行列との計算ルーチン実装(おそらく、別ファイル)
			};
		}
	#elif BOOST_PP_ITERATION_FLAGS() == 1
		// 同次元ベクトルとの演算を定義
		namespace spn {
			VT::VecT(const AVec& v) {
				STORETHIS(LOADPS(v.m)); }
			VT::VecT(const UVec& v) {
				STORETHIS(LOADPSU(v.m)); }

			VT& VT::operator = (const AVec& v) {
				STORETHIS(LOADPS(v.m));
				return *this;
			}
			VT& VT::operator = (const UVec& v) {
				STORETHIS(LOADPSU(v.m));
				return *this;
			}
			#define DEF_OP(op, func) \
					VT& VT::operator BOOST_PP_CAT(op,=) (float s) { \
						STORETHIS(func(LOADTHIS(), _mm_load_ps1(&s))); \
						return *this; } \
					VT VT::operator op (float s) const { \
						return VecT(func(LOADTHIS(), _mm_load1_ps(&s))); } \
					VT& VT::operator BOOST_PP_CAT(op,=) (const AVec& v) { \
						STORETHIS(func(LOADTHIS(), LOADPS(v.m))); \
						return *this; } \
					VT& VT::operator BOOST_PP_CAT(op,=) (const UVec& v) { \
						STORETHIS(func(LOADTHIS(), LOADPSU(v.m))); \
						return *this; } \
					VT::AVec VT::operator op (const AVec& v) const { \
						return AVec(func(LOADTHIS(), LOADPS(v.m))); } \
					VT::UVec VT::operator op (const UVec& v) const { \
						return UVec(func(LOADTHIS(), LOADPSU(v.m))); } \
					VT::AVec&& VT::operator op (AVec&& v) const { \
						STOREPS(v.m, func(LOADTHIS(), LOADPS(v.m))); \
						return std::forward<AVec>(v); } \
					VT::UVec&& VT::operator op (UVec&& v) const { \
						STOREPSU(v.m, func(LOADTHIS(), LOADPSU(v.m))); \
						return std::forward<UVec>(v); }
			DEF_OP(+, _mm_add_ps)
			DEF_OP(-, _mm_sub_ps)
			DEF_OP(*, _mm_mul_ps)
			DEF_OP(/, _mmDivPs)
			
			template <bool A>
			float VT::dot(const VecT<DIM,A>& v) const {
				return _sumup(_mm_mul_ps(LOADTHIS(), v.loadPS()));
			}
			float VT::average() const {
				return _sumup(LOADTHIS());
			}
			template <bool A>
			float VT::distance(const VecT<DIM,A>& v) const {
				return std::sqrt(dist_sq(v));
			}
			template <bool A>
			float VT::dist_sq(const VecT<DIM,A>& v) const {
				auto tv = v - *this;
				return tv.len_sq();
			}
			template <bool A>
			VT VT::getMin(const VecT<DIM,A>& v) const {
				return VT(_mm_min_ps(LOADTHIS(), v.loadPS())); }
			template <bool A>
			void VT::selectMin(const VecT<DIM,A>& v) {
				STORETHIS(_mm_min_ps(LOADTHIS(), v.loadPS())); }
			template <bool A>
			VT VT::getMax(const VecT<DIM,A>& v) const {
				return VT(_mm_max_ps(LOADTHIS(), v.loadPS())); }
			template <bool A>
			void VT::selectMax(const VecT<DIM,A>& v) {
				STORETHIS(_mm_max_ps(LOADTHIS(), v.loadPS())); }

			VT VT::operator - () const {
				return *this * -1.0f;
			}
			
			template <bool A>
			bool VT::operator == (const VecT<DIM,A>& v) const {
				__m128 r0 = _mm_cmpeq_ps(LOADTHIS(), v.loadPS());
				r0 = _mm_and_ps(r0, _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(1,0,3,2)));
				r0 = _mm_and_ps(r0, _mm_shuffle_ps(r0, r0, _MM_SHUFFLE(0,1,2,3)));
				return _mm_cvttps_pi32(r0) != 0;
			}

			void VT::normalize() {
				*this = normalization();
			}
			VT VT::normalization() const {
				float tmp = length();
				__m128 r0 = _mm_load_ps1(&tmp);
				r0 = _mm_div_ps(LOADTHIS(), r0);
				return VT(r0);
			}
			/*! \return ベクトルの長さ */
			float VT::length() const {
				return std::sqrt(len_sq());
			}
			/*! \return ベクトル長の2乗 */
			float VT::len_sq() const {
				__m128 r0 = LOADTHISZ();
				r0 = _mm_mul_ps(r0, r0);
				SUMVEC(r0)
				
				float ret;
				_mm_store_ss(&ret, r0);
				return ret;
			}
			void VT::saturate(float fMin, float fMax) {
				*this = saturation(fMin, fMax);
			}
			VT VT::saturation(float fMin, float fMax) const {
				__m128 xm = _mm_max_ps(LOADTHIS(), _mm_load1_ps(&fMin));
				return VT(_mm_min_ps(xm, _mm_load1_ps(&fMax)));
			}
			void VT::lerp(const UVec& v, float r) {
				*this = l_intp(v,r);
			}
			template <bool A>
			VT VT::l_intp(const VecT<DIM,A>& v, float r) const {
				_mm_mul_ps(_mm_load1_ps(&r), _mm_add_ps(LOADTHIS(), v.loadPS()));
			}
		}
	#else
		// 他次元ベクトルとの演算を定義
		namespace spn {
			#if ALIGN==1
				//! AVec -> Vec へ暗黙変換
				VT::operator VecT<DIM,false>& () { return *reinterpret_cast<VecT<DIM,false>*>(this); }
				VT::operator const VecT<DIM,false>& () const { return *reinterpret_cast<const VecT<DIM,false>*>(this); }
			#endif
			#if DIM==4
				#define _Vec3 VecT<3,BOOLNIZE(ALIGN)>
				/*! \return x,y,z成分 */
				_Vec3 VT::asVec3() const {
					return _Vec3(x,y,z);
				}
				/*! \return x,y,zそれぞれをwで割った値 */
				_Vec3 VT::asVec3Coord() const {
					__m128 r0 = _mm_rcp_ps(_mm_load_ps1(&w));
					return _Vec3(_mm_mul_ps(LOADTHIS(), r0));
				}
			#elif DIM==3
				/*! \return this X v の外積ベクトル */
				template <bool A>
				VT VT::cross(const VecT<DIM,A>& v) const {
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
					return VT(_mm_sub_ps(r0, r1));
				}
				//! 外積計算cross()と同義
				template <bool A>
				void VT::operator %= (const VecT<DIM,A>& v) {
					*this = cross(v);
				}
				//! 外積計算cross()と同義
				template <bool A>
				VT VT::operator % (const VecT<DIM,A>& v) const {
					return cross(v);
				}
				#define _Vec4 VecT<4,BOOLNIZE(ALIGN)>
				_Vec4 VT::asVec4(float w) const {
					return _Vec4(x,y,z,w);
				}
			#elif DIM==2
				// ccw
				template <bool A>
				float VT::ccw(const VecT<DIM,A>& v) const {
					return x*v.y - y*v.x;
				}
			#endif
			#undef Vec
			// 使いやすいようにクラスの別名を定義
			using BOOST_PP_CAT(BOOST_PP_IF(ALIGN,A,NOTHING), BOOST_PP_CAT(Vec,DIM)) = VT;
		}
	#endif
#endif
