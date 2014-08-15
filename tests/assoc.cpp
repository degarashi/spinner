#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "../assoc.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "serialization/pair.hpp"

namespace spn {
	namespace test {
		template <class K, class V, class C, class RDF>
		void AddRandom(AssocVecK<K,V,C>& a, int n, RDF& rdF) {
			while(--n >= 0)
				a.insert(rdF(), rdF());
		}
		template <class K, class C, class RDF>
		void AddRandom(AssocVec<K,C>& a, int n, RDF& rdF) {
			while(--n >= 0)
				a.insert(rdF());
		}
		template <class A, class RDR>
		void RemRandom(A& a, int n, RDR& rdR) {
			while(!a.empty() && --n >= 0) {
				int idx = rdR(0, a.size()-1);
				a.erase(idx);
			}
		}
		template <class A>
		void ChkSequence(const A& a, ...) {
			using C = typename A::cmp_type;
			C cmp;
			auto val = a[0];
			for(auto itr=a.cbegin() ; itr!=a.cend() ; itr++) {
				ASSERT_FALSE(cmp(*itr, val));
				val = *itr;
			}
		}
		template <class A>
		void ChkSequence(const A& a, decltype(std::declval<typename A::value_type>().first)* = nullptr) {
			using C = typename A::cmp_type;
			C cmp;
			auto val = a.getKey(0);
			for(auto itr=a.cbegin() ; itr!=a.cend() ; itr++) {
				ASSERT_FALSE(cmp(itr->first, val));
				val = itr->first;
			}
		}
		template <class A, class RD>
		void TestAssoc(RD& rd) {
			A	asv;
			auto rdF = [&rd](){ return rd.template getUniform<int>(); };
			auto rdR = [&rd](int from, int to){ return rd.template getUniformRange<int>(from,to); };

			// 適当な数の要素を追加(最低1回)
			AddRandom(asv, (rdF() & 0xf00) + 1, rdF);
			// 順番確認
			ChkSequence(asv, nullptr);
			// 適当なインデックスの要素を削除
			RemRandom(asv, rdR(0, asv.size()-1), rdR);
			ChkSequence(asv, nullptr);
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

		using AssocVectorSerialize = RandomTestInitializer;
		TEST_F(AssocVectorSerialize, WithoutKey) {
			// ランダムなデータ列を作る
			auto rd = this->getRand();
			auto rdF = [&rd](){ return rd.getUniform<int>(); };

			AssocVec<int, std::less<>> data;
			// 適当な数の要素を追加(最低1回)
			AddRandom(data, (rdF() & 0xf00) + 1, rdF);
			CheckSerializedDataBin(data);
		}
		TEST_F(AssocVectorSerialize, DISABLED_WithKey) {
			// ランダムなデータ列を作る
			auto rd = this->getRand();
			auto rdF = [&rd](){ return rd.getUniform<int>(); };

			AssocVecK<int, int, std::less<>> data;
			// 適当な数の要素を追加(最低1回)
			AddRandom(data, (rdF() & 0xf00) + 1, rdF);
			CheckSerializedDataBin(data);
		}
	}
}

