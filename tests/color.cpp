#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "color.hpp"

namespace spn {
	namespace test {
		class ColorTest : public RandomTestInitializer {};
		TEST_F(ColorTest, RGBtoHSVtoRGB) {
			auto rd = getRand();
			auto fnRand = [&rd](){ return rd.getUniformRange<float>(0.f, 1.f); };
			// RGB -> HSV -> RGBと変換して一致しているか
			RGBColor rgb0(fnRand(), fnRand(), fnRand());
			HSVColor hsv = rgb0.toHSV();
			RGBColor rgb1 = hsv.toRGB();

			EXPECT_NEAR(rgb0.x, rgb1.x, 1e-6f);
			EXPECT_NEAR(rgb0.y, rgb1.y, 1e-6f);
			EXPECT_NEAR(rgb0.z, rgb1.z, 1e-6f);

			// HSV -> RGBA が HSV -> RGBとAlpha以外一致しているか
			RGBAColor rgba1 = hsv.toRGBA(1.f);

			// ビット列までピッタリ同じ筈
			EXPECT_EQ(rgb1.x, rgba1.x);
			EXPECT_EQ(rgb1.y, rgba1.y);
			EXPECT_EQ(rgb1.z, rgba1.z);
			EXPECT_EQ(rgba1.w, 1.f);
		}
		TEST_F(ColorTest, HSVtoRGBtoHSV) {
			auto rd = getRand();
			auto fnRand = [&rd](){ return rd.getUniformRange<float>(0.f, 1.f); };
			// HSV -> RGB -> HSVと変換して一致しているか
			HSVColor hsv0(fnRand(), fnRand(), fnRand());
			RGBColor rgb = hsv0.toRGB();
			HSVColor hsv1 = rgb.toHSV();

			EXPECT_NEAR(hsv0.x, hsv1.x, 1e-6f);
			EXPECT_NEAR(hsv0.y, hsv1.y, 1e-6f);
			EXPECT_NEAR(hsv0.z, hsv1.z, 1e-6f);

			// RGB -> HSVA が RGB -> HSVとAlpha以外一致しているか
			HSVAColor hsva1 = rgb.toHSVA(1.f);

			// ビット列までピッタリ同じ筈
			EXPECT_EQ(hsv1.x, hsva1.x);
			EXPECT_EQ(hsv1.y, hsva1.y);
			EXPECT_EQ(hsv1.z, hsva1.z);
			EXPECT_EQ(hsva1.w, 1.f);
		}
	}
}

