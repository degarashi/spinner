#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "flyweight.hpp"

namespace spn {
	namespace test {
		class Flyweight : public RandomTestInitializer {};
		// template <class T>
		// class Flyweight : public RandomTestInitializer {};
		// using FlyweightT = ::testing::Types<>;
		TEST_F(Flyweight, General) {
			// キーに対して+100した物を生成物とする
			struct Gen {
				int operator()(const int key) const {
					return key+100;
				}
			};
			Gen gen;
			// ランダムにキーを生成
			auto rd = this->getRand();
			const auto rdF = [&rd](auto... a){ return rd.getUniform<int>(a...); };
			using Range = spn::Range<int>;

			// 確認用のキーと生成値
			struct Generated {
				int key;
				int	value;
				int	count;
			};
			using GeneratedV = std::vector<Generated>;
			GeneratedV gv;

			::spn::Flyweight<int, int, Gen> fw(gen);
			enum Check {
				Generate,		// 新たに値を生成
				Get,			// 既に存在する値を取得
				Put,			// 値を返却
				PutNotExist,	// 存在しない値をPut
				_Num
			};
			// チェックを実行する回数
			const int NCheck = rdF(Range{10, 1000});
			// 新たに値を生成する用のランダム配列
			std::vector<int> newindex(NCheck);
			std::iota(newindex.begin(), newindex.end(), 0);
			std::shuffle(newindex.begin(), newindex.end(), rd.refMt());
			for(int i=0 ; i<NCheck ; i++) {
				switch(rdF(Range{0, Check::_Num-1})) {
					case Check::Generate: {
						const int idx = newindex.back();
						newindex.pop_back();
						// まだ生成してないのでhas(index) == falseとなる
						ASSERT_FALSE(fw.has(idx));
						gv.emplace_back(Generated{idx, fw.get(idx), 1});
						// 生成したのでcount(index) == 1となる
						ASSERT_EQ(1, fw.count(idx));
						break;
					}
					case Check::Get: {
						const int s = gv.size();
						if(s == 0)
							break;
						const int idx = rdF(Range{0, s-1});
						auto& p = gv.at(idx);
						const int size = fw.size();
						ASSERT_EQ(p.count, fw.count(p.key));
						++p.count;
						ASSERT_EQ(p.value, fw.get(p.key));
						ASSERT_EQ(p.count, fw.count(p.key));
						ASSERT_EQ(size, fw.size());
						break;
					}
					case Check::Put: {
						const int s = gv.size();
						if(s == 0)
							break;
						const int idx = rdF(Range{0, s-1});
						auto& p = gv.at(idx);
						ASSERT_EQ(fw.count(p.key), p.count);
						fw.putKey(p.key);
						if(--p.count == 0)
							gv.erase(gv.begin() + idx);
						ASSERT_EQ(fw.count(p.key), p.count);
						break;
					}
					case Check::PutNotExist: {
						// 存在しない値に対してputしたら例外を送出
						const int num = rdF(Range{0, NCheck});
						ASSERT_THROW(fw.putValue(gen(num+NCheck)), std::invalid_argument);
						break;
					}
					default:
						__assume(0);
				}
			}
			if((rdF() & 1) == 0) {
				// 全ての登録値をput
				for(auto& p : gv) {
					for(;;) {
						if(p.count == 0)
							break;
						--p.count;
						fw.putKey(p.key);
					}
				}
			} else {
				fw.clear();
			}
			// サイズは0になっている筈
			ASSERT_EQ(0, fw.size());
		}
	}
}
