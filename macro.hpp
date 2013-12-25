#pragma once

#define BOOST_PP_VARIADICS 1
#include <boost/preprocessor.hpp>

//! 何も展開しない
#define NOTHING
//! 引数を任意個受け取り、何も展開しない
#define NOTHING_ARG(...)

//! 2の乗数シーケンス
#define SEQ_SHIFT	(1)(2)(4)(8)(16)(32)(64)(128)(256)
//! 右シフト
#define PP_RSHIFT(n,s)	BOOST_PP_DIV(n, BOOST_PP_SEQ_ELEM(s, SEQ_SHIFT))
//! 左シフト
#define PP_LSHIFT(n,s)	BOOST_PP_MUL(n, BOOST_PP_SEQ_ELEM(s, SEQ_SHIFT))
//! 指定の位置のビットを取り出す
#define PP_GETBIT(num,at)	BOOST_PP_MOD(PP_RSHIFT(num,at), 2)

// --------------- 指定クラスのイテレータ関数をそのまま呼ぶ ---------------
// bit0		[_][c][_][_][c][_]...
// bit0|1	[_][c][c][_][c][c]...
// bit2		[_][_][_][r][r][r]...
// bit3		[b][b][b][b][b][b][e][e][e][e][e][e]...
// 但しbit0&1が1の時はskip

#define ITR_ADAPTOR_LOCAL_2(src, itrname, func, c_post) \
				itrname func() c_post { return src.func(); }
#define ITR_ADAPTOR_LOCAL_1(src, c_pre, ci_pre, c_post, r_pre, ri_pre, dir) \
				ITR_ADAPTOR_LOCAL_2(src, \
				BOOST_PP_CAT(ci_pre, BOOST_PP_CAT(ri_pre, iterator)), \
				BOOST_PP_CAT(c_pre, BOOST_PP_CAT(r_pre, dir)), \
				c_post)
#define ITR_ADAPTOR_LOCAL_0(src, b0,b1,b2,b3) \
				ITR_ADAPTOR_LOCAL_1(src, \
				BOOST_PP_IF(b0, c, NOTHING), \
				BOOST_PP_IF(BOOST_PP_BITOR(b1,b0), const_, NOTHING), \
				BOOST_PP_IF(BOOST_PP_BITOR(b1,b0), const, NOTHING), \
				BOOST_PP_IF(b2, r, NOTHING), \
				BOOST_PP_IF(b2, reverse_, NOTHING), \
				BOOST_PP_IF(b3, end, begin))
//! b0とb1が共に1ならば何も展開しない
#define ITR_CLIP_INVALID(src,b0,b1,b2,b3)	BOOST_PP_IF(BOOST_PP_BITAND(b0, b1), NOTHING_ARG, ITR_ADAPTOR_LOCAL_0)(src,b0,b1,b2,b3)
//! nの下位4ビットを展開してCLIP_INVALIDに渡す
#define ITERATOR_ADAPTOR(z,n,src)			ITR_CLIP_INVALID(src, PP_GETBIT(n,0), PP_GETBIT(n,1), PP_GETBIT(n,2), PP_GETBIT(n,3))
//! 大文字アルファベットシーケンス
#define SEQ_ALPHABET (A)(B)(C)(D)(E)(F)(G)(H)(I)(J)(K)(L)(M)(N)(O)(P)(Q)(R)(S)(T)(U)(V)(W)(X)(Y)(Z)
//! tupleのsequenceを定義
#define MAKE_SEQ(size, rel) MAKE_SEQ_D(size, rel)
#define MAKE_SEQ_D(size, rel) \
	BOOST_PP_CAT( \
		MAKE_SEQ_A_ ## size rel, \
		0X0 \
	)() \
	/**/

// size 2
#define MAKE_SEQ_A_2(x, y) ((x, y)) MAKE_SEQ_B_2
#define MAKE_SEQ_B_2(x, y) ((x, y)) MAKE_SEQ_A_2
// size 3
#define MAKE_SEQ_A_3(x, y, z) ((x, y, z)) MAKE_SEQ_B_3
#define MAKE_SEQ_B_3(x, y, z) ((x, y, z)) MAKE_SEQ_A_3
// size 4
#define MAKE_SEQ_A_4(x, y, z, w) ((x, y, z, w)) MAKE_SEQ_B_4
#define MAKE_SEQ_B_4(x, y, z, w) ((x, y, z, w)) MAKE_SEQ_A_4

#define MAKE_SEQ_A_20X0()
#define MAKE_SEQ_B_20X0()
#define MAKE_SEQ_A_30X0()
#define MAKE_SEQ_B_30X0()
#define MAKE_SEQ_A_40X0()
#define MAKE_SEQ_B_40X0()

// Move CtorでMoveするオブジェクトをSequenceによって指定
#define Move_CtorInner(z, obj, elem)	(elem(std::move(obj.elem)))
#define Move_Ctor(seq, obj)				BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(Move_CtorInner, obj, seq))
#define Move_AssignInner(z, obj, elem)	elem = std::move(obj.elem);
#define Move_Assign(seq, obj)			BOOST_PP_SEQ_FOR_EACH(Move_AssignInner, obj, seq)
