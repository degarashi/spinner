#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "../structure/valuearray.hpp"
#include <numeric>

namespace spn {
	namespace test {
		template <class IC>
		class ValueArrayTest : public RandomTestInitializer {};
		using VA_t = ::testing::Types<std::integral_constant<int, 1>,
									std::integral_constant<int, 2>,
									std::integral_constant<int, 4>,
									std::integral_constant<int, 8>,
									std::integral_constant<int, 16>,
									std::integral_constant<int, 32>>;
		TYPED_TEST_CASE(ValueArrayTest, VA_t);
		TYPED_TEST(ValueArrayTest, Compare) {
			auto rd = this->getRand();
			auto fnRand = [&rd](){ return rd.template getUniformMin<int>(0); };

			// ランダムデータを生成
			using Var = ValueArray<int, TypeParam::value>;
			Var ar0, ar1;
			auto fnMakeArray = [&fnRand](auto& ar){
				for(auto& a : ar.value)
					a = fnRand();
			};
			fnMakeArray(ar0);

			// Check operator ==
			EXPECT_EQ(ar0, ar0);
			// Check operator =
			ar1 = ar0;
			EXPECT_EQ(ar0, ar1);
			// Check operator <
			// 数値を1ついじって大小比較
			int pos = fnRand() % Var::length;
			auto& data = ar0.value[pos];
			if(data == 0) {
				++data;
				EXPECT_GT(ar0, ar1);
			} else {
				--data;
				EXPECT_LT(ar0, ar1);
			}
			// その他の比較演算子はboostが定義しているので、ここではテスト対象としない

			// Check copy constructor
			Var ar2(ar0);
			EXPECT_EQ(ar0, ar2);
		}
	}
}

