#include "unittest.hpp"
#include <random>
#include <array>
#include <limits>
#include <cassert>
#include <deque>

namespace spn {
	namespace unittest {
		void PQueue() {
			using Data = MoveOnly<MoveOnly<int>>;
			std::random_device rdev;
			std::array<uint_least32_t, 32> seed;
			std::generate(seed.begin(), seed.end(), std::ref(rdev));
			std::seed_seq seq(seed.begin(), seed.end());
			std::mt19937 mt(seq);
			std::uniform_int_distribution<int> uni{0, std::numeric_limits<int>::max()};

			pqueue<Data, std::deque> q;
			// 並び順のチェック
			auto fnCheck = [&q]() {
				int check = 0;
				for(auto& a : q) {
					assert(check <= a.getValue());
					check = a.getValue();
				}
			};
			// ランダムな値を追加
			auto fnAddrand = [&](int n) {
				for(int i=0 ; i<n ; i++)
					q.push(Data(uni(mt)));
			};

			fnAddrand(512);
			fnCheck();

			// 先頭の半分を削除して、再度チェック
			for(int i=0 ; i<256 ; i++)
				q.pop_front();
			fnAddrand(256);
			fnCheck();
			assert(q.size() == 512);
		}
	}
}
