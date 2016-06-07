#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "sharedflyweight.hpp"

namespace spn {
	namespace test {
		class SFlyweight : public RandomTestInitializer {};
		TEST_F(SFlyweight, General) {
			// キーに対して+100した物を生成物とする
			struct Gen {
				auto operator()(const int key) const {
					return std::make_shared<int>(key+100);
				}
			};
			Gen gen;
			// ランダムにキーを生成
			auto rd = this->getRand();
			const auto rdF = [&rd](auto... a){ return rd.getUniform<int>(a...); };
			using Range = spn::Range<int>;

			using sp_t = std::shared_ptr<int>;
			using spv_t = std::vector<sp_t>;
			// 確認用のキーと生成値
			struct Generated {
				int		key;
				spv_t	value;
			};
			using GeneratedV = std::vector<Generated>;
			GeneratedV gv;

			::spn::SharedFlyweight<int, int, Gen> fw(gen);
			enum Check {
				Generate,		// 新たに値を生成
				Get,			// 既に存在する値を取得
				Put,			// 既に存在する値を解放
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
						gv.emplace_back(Generated{idx, spv_t{}});
						gv.back().value.emplace_back(fw.get(idx));
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
						// 既にある要素なのでGet前と後で数は変わらない
						const int size = fw.size();
						// 値のカウンタが一致しているか
						ASSERT_EQ(int(p.value.size()), fw.count(p.key));
						p.value.emplace_back(fw.get(p.key));
						// ちゃんと値が取れているか
						for(auto& v : p.value)
							ASSERT_EQ(v, p.value.front());
						ASSERT_EQ(int(p.value.size()), fw.count(p.key));
						ASSERT_EQ(size, fw.size());
						break;
					}
					case Check::Put: {
						const int s = gv.size();
						if(s == 0)
							break;
						const int idx = rdF(Range{0, s-1});
						auto& p = gv.at(idx);
						ASSERT_EQ(int(p.value.size()), fw.count(p.key));
						p.value.pop_back();
						ASSERT_EQ(int(p.value.size()), fw.count(p.key));
						if(p.value.empty())
							gv.erase(gv.begin()+idx);
						break;
					}
					default:
						__assume(0);
				}
			}
			if((rdF() & 1) == 0) {
				// 全ての登録値をrelease
				gv.clear();
			} else {
				fw.clear();
			}
			// サイズは0になっている筈
			ASSERT_EQ(0, fw.size());
		}
	}
}
