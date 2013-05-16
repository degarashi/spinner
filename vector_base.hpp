//! ベクトルクラス基底ヘッダ
/*! 2-4次元のベクトルをアラインメント有りと無しでそれぞれ定義
	初期化や要素毎のアクセスなど最低限の機能のみ */

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
		
		template <int N, bool A>
		struct VecT;
		// アラインメント無し
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (2,4, "vector_base.hpp", 0))
		#include BOOST_PP_ITERATE()
		// 16byteアラインメント有り
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (2,4, "vector_base.hpp", 1))
		#include BOOST_PP_ITERATE()
	#endif
#else
	#define DIM		BOOST_PP_ITERATION()
	#define ALIGN	BOOST_PP_ITERATION_FLAGS_1()
		
	template <>
	struct BOOST_PP_IF(ALIGN, alignas(16), NOTHING) VecT<DIM, BOOLNIZE(ALIGN)> {
		union {
			struct {
				float BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, DIM));
			};
			float m[DIM];
		};
		VecT() = default;
		DEF_REGSET(ALIGN,DIM)
		VecT(ENUM_ARGPAIR(BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, DIM))) {
			BOOST_PP_SEQ_FOR_EACH(DEF_ARGSET, NOTHING, BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, DIM))
		}
		__m128 loadPS() const {
			return BOOST_PP_IF(ALIGN, _mm_load_ps, _mm_loadu_ps)(m);
		}
	};
#endif
