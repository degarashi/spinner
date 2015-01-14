#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "assoc.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace spn {
	namespace test {
		template <class A, class RD>
		void TestAssoc(RD& rd) {
			A	asv;
			auto rdF = [&rd](){ return rd.template getUniform<int>(); };
			auto rdR = [&rd](int from, int to){ return rd.template getUniform<int>({from,to}); };

			// 適当な数の要素を追加(最低1回)
			AddRandom((rdF() & 0xf00) + 1, rdF, asv);
			// 順番確認
			ASSERT_NO_FATAL_FAILURE(ChkSequence(asv, nullptr));
			// 適当なインデックスの要素を削除
			RemRandom(rdR(0, asv.size()-1), rdR, asv);
			ASSERT_NO_FATAL_FAILURE(ChkSequence(asv, nullptr));
		}

		template <class T>
		class AssocVectorSequence : public RandomTestInitializer {};
		using AssocT = ::testing::Types<std::less<>, std::greater<>>;
		TYPED_TEST_CASE(AssocVectorSequence, AssocT);
		TYPED_TEST(AssocVectorSequence, WithoutKey) {
			auto rd = this->getRand();
			TestAssoc<AssocVec<int,TypeParam>>(rd);
		}
		TYPED_TEST(AssocVectorSequence, WithKey) {
			auto rd = this->getRand();
			TestAssoc<AssocVecK<int,int,TypeParam>>(rd);
		}

		template <class T>
		using AssocVectorModify = AssocVectorSequence<T>;
		TYPED_TEST_CASE(AssocVectorModify, AssocT);
		TYPED_TEST(AssocVectorModify, WithoutKey) {
			auto rd = this->getRand();
			auto rdF = [&rd](){ return rd.template getUniformMin<int>(0); };
			AssocVec<int, TypeParam> asv;
			// 適当な数の要素を追加
			AddRandom(rdF() % 0xf00, rdF, asv);
			// 何箇所か適当に値を改変
			int n = rdF() % 0x10;
			auto* ptr = const_cast<int*>(&asv.front());
			while(n-- != 0) {
				int pos = rdF() % asv.size();
				ptr[pos] ^= 0xf0f0f0f0;
			}
			// 再度ソートをかける
			asv.re_sort();
			// 順番確認
			ASSERT_NO_FATAL_FAILURE(ChkSequence(asv, nullptr));
		}

		using AssocVectorSerialize = RandomTestInitializer;
		TEST_F(AssocVectorSerialize, WithoutKey) {
			// ランダムなデータ列を作る
			auto rd = this->getRand();
			auto rdF = [&rd](){ return rd.getUniform<int>(); };

			AssocVec<int, std::less<>> data;
			// 適当な数の要素を追加(最低1回)
			AddRandom((rdF() & 0xf00) + 1, rdF, data);
			CheckSerializedDataBin(data);
		}
		TEST_F(AssocVectorSerialize, WithKey) {
			// ランダムなデータ列を作る
			auto rd = this->getRand();
			auto rdF = [&rd](){ return rd.getUniform<int>(); };

			AssocVecK<int, int, std::less<>> data;
			// 適当な数の要素を追加(最低1回)
			AddRandom((rdF() & 0xf00) + 1, rdF, data);
			CheckSerializedDataBin(data);
		}
	}
}

