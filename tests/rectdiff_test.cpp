#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "../rectdiff.hpp"

namespace spn {
	namespace test {
		namespace {
			void CheckRect(const Rect& r) {
				ASSERT_LE(r.x0, r.x1);
				ASSERT_LE(r.y0, r.y1);
			}
			template <class RF, class RF2>
			Rect RandomRect(RF& rfw, RF2& rfh) {
				const auto x0 = rfw(),
							x1 = rfw(),
							y0 = rfh(),
							y1 = rfh();
				return Rect(std::min(x0, x1), std::max(x0, x1),
							std::min(y0, y1), std::max(y0, y1));
			}
			template <class RF, class RF2>
			Rect RandomRectNZ(RF& rfw, RF2& rfh) {
				auto r = RandomRect(rfw, rfh);
				if(r.width() == 0)
					r.x1 += 1;
				if(r.height() == 0)
					r.y1 += 1;
				return r;
			}
		}
		class RectDiff : public RandomTestInitializer {};
		TEST_F(RectDiff, Divide) {
			// ランダムなテスト範囲
			auto rd = getRand();
			auto randI = rd.template getUniformF<int>();
			constexpr int MaxRange = 100;
			const Size size(randI({1, MaxRange}),
							randI({1, MaxRange}));
			bool imap[MaxRange][MaxRange];
			auto randW = rd.template getUniformF<int>({1, size.width}),
				randH = rd.template getUniformF<int>({1, size.height});
			constexpr int NItr = 0x100;
			for(int i=0 ; i<NItr ; i++) {
				std::memset(imap, 0, sizeof(imap));
				// テスト範囲以下の矩形をランダムな位置に置く
				const auto rect = RandomRect(randW, randH);
				int area = 0;
				ASSERT_NO_FATAL_FAILURE(
					rect::DivideRect(
						size,
						rect,
						[&rect, &size, &area, &imap](const Rect& g, const Rect& l){
							ASSERT_NO_FATAL_FAILURE(CheckRect(g));
							ASSERT_NO_FATAL_FAILURE(CheckRect(l));
							// グローバルとローカル矩形の面積は同じ
							ASSERT_EQ(g.area(), l.area());
							// グローバル座標が入力値と一致しているか
							ASSERT_EQ(rect, g);
							// ローカル座標がテスト範囲を出ていないか
							ASSERT_TRUE(IsInRange(l.x0, 0, size.width));
							ASSERT_TRUE(IsInRange(l.x1, 0, size.width));
							ASSERT_TRUE(IsInRange(l.y0, 0, size.height));
							ASSERT_TRUE(IsInRange(l.y1, 0, size.height));
							// 矩形を塗りつぶしていき、重なりや面積が違っていなければOK
							for(int i=g.y0 ; i<g.y1 ; i++) {
								for(int j=g.x0 ; j<g.x1 ; j++) {
									auto& im = imap[i][j];
									ASSERT_FALSE(im);
									im = true;
								}
							}
							area += g.area();
						}
					)
				);
				ASSERT_EQ(rect.area(), area);
			}
		}
		TEST_F(RectDiff, Incremental) {
			// ランダムなテスト範囲
			auto rd = getRand();
			constexpr int MaxRange = 100;
			const auto randMove = rd.template getUniformF<int>({-MaxRange, MaxRange});
			const auto randW = rd.template getUniformF<int>({1, MaxRange});
			constexpr int NItr = 0x100;
			for(int i=0 ; i<NItr ; i++) {
				// テスト範囲以下の矩形をランダムな位置に置く
				const auto rect = RandomRectNZ(randW, randW);
				// 矩形を移動
				const int mvx = randMove(),
						mvy = randMove();
				const auto rect2 = rect.move(mvx, mvy);
				const bool bHit = rect.hit(rect2);
				int area0 = 0;
				int nResult = 0;
				Rect result[2];
				rect::IncrementalRect(
					rect,
					mvx,
					mvy,
					[&area0, &result, &nResult, &rect, &rect2, bHit](const Rect& r){
						if(nResult != 0) {
							// 既に算出された差分矩形同士は重ならない
							ASSERT_FALSE(r.hit(result[0]));
						}
						// 元の矩形と差分矩形は重なっていない
						ASSERT_FALSE(r.hit(rect));
						// 移動後の矩形と差分矩形は重なる
						ASSERT_TRUE(r.hit(rect2));

						area0 += r.area();
						result[nResult++] = r;
					}
				);
				// 前後それぞれの矩形の面積 - 重なっている矩形の面積 == 差分矩形の面積
				int area1;
				if(const auto cr = rect.cross(rect2)) {
					area1 = rect.area() - cr->area();
				} else
					area1 = rect.area();
				ASSERT_EQ(area1, area0);
			}
		}
	}
}
