#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "../structure/profiler.hpp"

namespace spn {
	namespace test {
		class ProfilerTest : public RandomTestInitializer {
			protected:
				void SetUp() override {
					profiler.clear();
					profiler.reInitialize();
					RandomTestInitializer::SetUp();
				}
		};
		/*	各ブロックの実行に掛かった時間の正確性をチェックするのは
			テスト中に意図せぬ負荷が掛かった場合にコケるので無し
			ツリーの接合性 (累積時間や呼び出し回数)のみが対象 */
		TEST_F(ProfilerTest, General) {
			auto rd = getRand();
			const int nBlock = rd.getUniform<int>({4,32});
			struct Block {
				prof::Profiler::Name	name;
				uint32_t				nCalled;
			};
			std::vector<std::string>	nameV(nBlock);
			std::vector<Block>	block(nBlock);
			for(int i=0 ; i<nBlock ; i++) {
				nameV[i] = (boost::format("block_%1%") % i).str();
				block[i].name = nameV[i].c_str();
				block[i].nCalled = 0;
			}
			std::vector<prof::Profiler::Name> nameStack;
			// ランダムな回数、ランダムなブロック名を入力
			int nIter = rd.getUniform<int>({1,64});
			while(nIter-- > 0) {
				const int index = rd.getUniform<int>({-(nBlock/2+1),nBlock-1});
				if(index < 0) {
					// 上の階層に上がる
					if(!nameStack.empty()) {
						profiler.endBlock(nameStack.back());
						nameStack.pop_back();
					}
				} else {
					auto& blk = block[index];
					++blk.nCalled;
					profiler.beginBlock(blk.name);
					nameStack.push_back(blk.name);
				}
			}
			while(!nameStack.empty()) {
				profiler.endBlock(nameStack.back());
				nameStack.pop_back();
			}

			prof::USec sum(0);
			const bool bf = profiler.onFrame();
			if(!bf)
				profiler.endBlock(prof::Profiler::DefaultRootName);
			const auto& ci = bf ? profiler.getPrev() : profiler.getCurrent();
			const auto& root = ci.root;
			if(root) {
				root->iterateDepthFirst<false>([&sum](const auto& blk, int){
					// 下位ブロックの合計は自ブロックの時間を超えない
					const auto lowerTime = blk.getLowerTime();
					EXPECT_GE(blk.hist.tAccum, lowerTime);
					sum += blk.hist.tAccum - lowerTime;
					return Iterate::StepIn;
				});
				// ルートブロックの累積時間は全てのブロックの個別時間を合わせたものと"ほぼ"等しい
				ASSERT_NEAR(root->hist.tAccum.count(),  sum.count(), sum.count()/100+1);
				// 同じ名前のブロックを合算 => 呼び出し回数を比較
				for(int i=0 ; i<nBlock ; i++) {
					const auto& blk = block[i];
					if(blk.nCalled == 0) {
						ASSERT_EQ(0, ci.byName.count(blk.name));
					} else {
						ASSERT_EQ(ci.byName.count(blk.name), 1);
						const auto res = ci.byName.at(blk.name);
						ASSERT_EQ(res.nCalled, blk.nCalled);
					}
				}
			} else {
				for(int i=0 ; i<nBlock ; i++) {
					ASSERT_EQ(0, block[i].nCalled);
				}
			}
		}
	}
}
