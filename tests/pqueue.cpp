#include "test.hpp"
#include "pqueue.hpp"
#include "../random.hpp"

namespace spn {
	namespace test {
		void PQueue() {
			struct MyPair {
				int a,b;
				bool operator < (const MyPair& my) const {
					return a < my.a;
				}
			};
			{
				spn::pqueue<MyPair, std::deque, std::less<MyPair>, InsertBefore> myB;
				for(int i=0 ; i<20 ; i++)
					myB.push(MyPair{0, i*100});

				int check = myB.front().b;
				for(auto& me : myB) {
					assert(check >= me.b);
					check = me.b;
				}
			}
			{
				spn::pqueue<MyPair, std::deque, std::less<MyPair>, InsertAfter> myA;
				for(int i=0 ; i<20 ; i++)
					myA.push(MyPair{0, i*100});
				int check = myA.front().b;
				for(auto& me : myA) {
					assert(check <= me.b);
					check = me.b;
				}
			}

			using Data = MoveOnly<MoveOnly<int>>;
			auto rd = mgr_random.get(0);
			pqueue<Data, std::deque, std::less<Data>, InsertBefore> q;
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
					q.push(Data(rd.getUniformMin(0)));
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

		void PrintReg(reg128 r) {
			float t[4];
			reg_store_ps(t, r);
			for(int i=0 ; i<4 ; i++)
				std::cout << t[i] << std::endl;
		}
		void Math() {
			std::cout << "---- math test ----" << std::endl << spn::Rcp22Bit(128) << std::endl;
			reg128 ra, rb;
			ra = reg_set_ps(1,2,3,4);
			rb = reg_set_ps(123,456,789,901);
			ra = _mmDivPs(ra, rb);
			ra = reg_div_ps(ra, rb);
			ra = reg_add_ps(ra, rb);
			ra = reg_mul_ps(ra, rb);
			ra = reg_sub_ps(ra, rb);
			ra = reg_or_ps(ra, ra);
 			rb = reg_xor_ps(ra, rb);

			PrintReg(ra);
			PrintReg(rb);
		}
	}
}
