#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "../sort.hpp"
#include "../structure/range.hpp"
#include <list>

namespace spn {
	namespace test {
		namespace {
			template <class A, class CMP>
			void ChkSequence(const A& a, const CMP& cmp) {
				if(a.size() < 2)
					return;

				auto* pVal = &a.front();
				for(auto itr=std::next(a.cbegin()) ; itr!=a.cend() ; itr++) {
					ASSERT_FALSE(cmp(*itr, *pVal));
					pVal = &(*itr);
				}
			}
			template <class T, class AR, class CMP, class RD, class SORT>
			void SortTest(RD& rd, const SORT& sf) {
				auto rdF = [&](){ return rd.template getUniform<int>(
							RangeI{0, std::numeric_limits<int>::max()}); };

				// ランダムなデータ列を作る
				auto nItem = rdF() % 0xf00;
				AR ar;
				for(int i=0 ; i<nItem ; i++)
					ar.emplace_back(rdF());

				// ソートをかける
				sf(ar, ar.begin(), ar.end(), CMP());
				// 順番確認
				ASSERT_NO_FATAL_FAILURE(ChkSequence(ar, CMP()));
			}
			auto g_fnSort = [](auto& /*lst*/, auto&&... arg){
				insertion_sort(std::forward<std::decay_t<decltype(arg)>>(arg)...);
			};
			auto g_fnSortL = [](auto& lst, auto&&... arg){
				insertion_sort_list(lst, std::forward<std::decay_t<decltype(arg)>>(arg)...);
			};
		}

		template <class T>
		class InsertionSort : public RandomTestInitializer {};
		using IS_t = ::testing::Types<std::less<>, std::greater<>>;
		TYPED_TEST_CASE(InsertionSort, IS_t);
		TYPED_TEST(InsertionSort, Primitive) {
			auto rd = this->getRand();
			SortTest<int, std::vector<int>, TypeParam>(rd, g_fnSort);
		}
		TYPED_TEST(InsertionSort, Moveonly) {
			auto rd = this->getRand();
			SortTest<MoveOnly<int>, std::vector<MoveOnly<int>>, TypeParam>(rd, g_fnSort);
		}

		template <class T>
		using InsertionSortList = InsertionSort<T>;
		TYPED_TEST_CASE(InsertionSortList, IS_t);
		TYPED_TEST(InsertionSortList, Primitive) {
			auto rd = this->getRand();
			SortTest<int, std::list<int>, TypeParam>(rd, g_fnSortL);
		}
		TYPED_TEST(InsertionSortList, Moveonly) {
			auto rd = this->getRand();
			SortTest<MoveOnly<int>, std::list<MoveOnly<int>>, TypeParam>(rd, g_fnSortL);
		}
	}
}

