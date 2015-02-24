#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "emplace.hpp"

namespace spn {
	namespace test {
		template <class M, class FK, class FD>
		std::pair<M,M> MakeRandomData(int nData, FK&& fKey, FD&& fData) {
			M m0, m1;
			using Data = typename M::mapped_type;
			while(nData-- > 0) {
				auto key = std::forward<FK>(fKey)();
				while(m0.count(key) != 0)
					key = std::forward<FD>(fData)();

				auto data = std::forward<FD>(fData)();
				m0.emplace(key, Data(data));
				m1.emplace(key, Data(data));
			}
			return std::make_pair(std::move(m0), std::move(m1));
		}

		template <class T>
		class EmplaceTest : public RandomTestInitializer {};
		using EmplaceT = ::testing::Types<std::pair<int,int>, std::pair<int,MoveOnly<int>>>;
		TYPED_TEST_CASE(EmplaceTest, EmplaceT);
		TYPED_TEST(EmplaceTest, TryEmplace) {
			auto rd = this->getRand();
			auto rdF = [&rd](int n){ return rd.template getUniform<int>({1, n}); };
			auto rdKD = [&rd](){ return rd.template getUniform<int>(); };

			// ランダムなデータをペアで作成
			using Ft = typename TypeParam::first_type;
			using St = typename TypeParam::second_type;
			auto mp = MakeRandomData<std::unordered_map<Ft,St>>(rdF(0x100), rdKD, rdKD);

			// TryEmplaceでキーの存在しないエントリだけを追加 = emplace()と同じだが内部でオブジェクトの作成をしない
			int n = rdF(0x100);
			for(int i=0 ; i<n ; i++) {
				int key=rdKD(),
					data=rdKD();
				mp.first.emplace(key, St(data));
				TryEmplace(mp.second, key, data);
			}
			// 結果を比較
			ASSERT_EQ(mp.first, mp.second);
		}
		TYPED_TEST(EmplaceTest, EmplaceOrReplace) {
			auto rd = this->getRand();
			auto rdF = [&rd](int n){ return rd.template getUniform<int>({1, n}); };
			auto rdKD = [&rd](){ return rd.template getUniform<int>(); };

			// ランダムなデータをペアで作成
			using Ft = typename TypeParam::first_type;
			using St = typename TypeParam::second_type;
			auto mp = MakeRandomData<std::unordered_map<Ft,St>>(rdF(0x100), rdKD, rdKD);

			// EmplaceOrReplaceでそれぞれエントリを上書き = operator[] と同じだがemplaceと同じように記述できる
			int n = rdF(0x100);
			for(int i=0 ; i<n ; i++) {
				int key=rdKD(),
					data=rdKD();
				auto itr = mp.first.find(key);
				if(itr != mp.first.end())
					mp.first.erase(itr);
				mp.first.emplace(key, St(data));
				EmplaceOrReplace(mp.second, key, data);
			}
			// 結果を比較
			ASSERT_EQ(mp.first, mp.second);
		}
	}
}
