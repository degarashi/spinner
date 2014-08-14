#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "../assoc.hpp"

namespace spn {
	namespace test {
		using AssocVector = RandomTestInitializer;
		TEST_F(AssocVector, WithoutKey) {
			AssocVec<int, std::less<>> av_asc;
			AssocVec<int, std::greater<>> av_desc;
			auto rd = this->getRand();
			auto rdF = [&rd](){ return rd.getUniform<int>(); };
			auto rdR = [&rd](int from, int to){ return rd.getUniformRange<int>(from,to); };

			// 適当な数の要素を追加(最低1回)
			int nAdd = (rdF() & 0xf00) + 1;
			for(int i=0 ; i<nAdd ; i++) {
				av_asc.insert(rdF());
				av_desc.insert(rdF());
			}
			// 昇順になっているか確認
			auto fnChkAsc = [&av_asc](){
				int val = std::numeric_limits<int>::min();
				for(auto itr=av_asc.begin() ; itr!=av_asc.end() ; itr++) {
					ASSERT_LE(val, *itr);
					val = *itr;
				}
			};
			// 降順になっているか確認
			auto fnChkDesc = [&av_desc](){
				int val = std::numeric_limits<int>::max();
				for(auto itr=av_desc.begin() ; itr!=av_desc.end() ; itr++) {
					ASSERT_GE(val, *itr);
					val = *itr;
				}
			};
			fnChkAsc();
			fnChkDesc();

			// 適当なインデックスの要素を削除
			int nRem = rdR(0, av_asc.size()-1);
			while(!av_asc.empty() && nRem>0) {
				int idx = rdR(0, av_asc.size()-1);
				av_asc.erase(idx);
				av_desc.erase(idx);
				--nRem;
			}
			fnChkAsc();
			fnChkDesc();
		}
	}
}

