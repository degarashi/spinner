#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "assoc.hpp"
#include "../structure/mapvec.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <vector>
#include <deque>
#include <set>

namespace spn {
	namespace test {
		namespace {
			struct CmpFirst {
				template <class F, class S>
				bool operator ()(const std::pair<F,S>* p0, const std::pair<F,S>* p1) const {
					return p0->first < p1->first;
				}
				template <class V, std::enable_if_t<!std::is_pointer<V>::value>& = Enabler>
				bool operator ()(const V& v0, const V& v1) const { return v0 < v1; }
				template <class V>
				bool operator ()(const V* v0, const V* v1) const { return *v0 < *v1; }
			};
			template <class T0, class T1>
			void CheckEqual(const T0& t0, const T1& t1) {
				ASSERT_EQ(t0.size(), t1.size());
				auto itr0 = t0.cbegin();
				auto itr1 = t1.cbegin();
				while(itr0 != t0.cend()) {
					ASSERT_EQ(*(*itr0), *itr1);
					++itr0;
					++itr1;
				}
			}
		}

		using DataType = int;
		using SetType = std::set<DataType>;
		template <template <class,class> class CT>
		struct Container_t {
			template <class M>
			using GetType = spn::AssocVec<typename M::value_type*, CmpFirst>;
		};
		template <class T>
		class MapVecTest : public RandomTestInitializer {};
		using MapVecT = ::testing::Types<Container_t<std::vector>, Container_t<std::deque>>;
		TYPED_TEST_CASE(MapVecTest, MapVecT);
		TYPED_TEST(MapVecTest, Test) {
			auto rd = this->getRand();
			auto rdF = [&rd](){ return rd.template getUniform<DataType>(); };
			auto rdR = [&rd](int from, int to){ return rd.template getUniform<int>({from,to}); };

			MapVec<SetType, typename TypeParam::template GetType<SetType>>	mv;		// テストするデータ構造
			AssocVec<DataType, std::less<>> chk;	// チェック用のデータ構造

			// Mapを参照し、データをランダムに追加
			AddRandomV((rdF() & 0x400) + 1, rdF, *mv.refMap(), chk);
			ASSERT_NO_FATAL_FAILURE(CheckEqual(*mv.refVec(), chk));
			// Vectorを参照し、データをランダムに削除
			RemRandom(rdF() & 0x80, rdR, *mv.refVec(), chk);
			ASSERT_NO_FATAL_FAILURE(CheckEqual(*mv.refVec(), chk));
			// Mapを参照し、データをランダムに削除
			RemRandom(rdF() & 0x80, rdR, *mv.refMap(), chk);
			ASSERT_NO_FATAL_FAILURE(CheckEqual(*mv.refVec(), chk));
		}
	}
}

