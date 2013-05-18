//! ベクトルクラス基底ヘッダ
/*! 2-4次元のベクトルをアラインメント有りと無しでそれぞれ定義
	初期化や要素毎のアクセスなど基本的なメソッド */

#if !BOOST_PP_IS_ITERATING
	#ifndef VECTOR_BASE_HEADER_
		#define VECTOR_BASE_HEADER_
		#define BOOST_PP_VARIADICS 1
		#include <boost/preprocessor.hpp>
		#define DEF_ARGPAIR(index,data,elem)	(float BOOST_PP_CAT(f, elem))
		#define ENUM_ARGPAIR(subseq) BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(DEF_ARGPAIR, NOTHING, subseq))
		#define DEF_ARGSET(index,data,elem)		elem = BOOST_PP_CAT(f, elem);
		#define DEF_REGSET(align, n)	VecT(__m128 r){ BOOST_PP_CAT(BOOST_PP_CAT(STOREPS_, AFLAG(align)), n)(m, r); }
		#define SEQ_VECELEM (x)(y)(z)(w)
		namespace spn {
			template <int N, bool A>
			struct VecT;
		}
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (2,4, "vector_base.hpp", 0))
		#include BOOST_PP_ITERATE()
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (2,4, "vector_base.hpp", 1))
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
		// クラスとコンストラクタだけ定義
		namespace spn {
			template <>
			struct Vec {
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
				DEF_REGSET(ALIGN,DIM)
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
		}
	#else
		// 他次元ベクトルとの演算を定義
		// 使いやすいようにクラスの別名を定義
	#endif
#endif
