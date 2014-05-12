#include "test.hpp"
#include "serialization.hpp"
#include "../random.hpp"

namespace spn {
	namespace test {
		void SerializeTest_noseq() {
			auto rd = mgr_random.get(0);
			std::stringstream buffer;
			for(int i=0 ; i<NTEST ; i++) {
				buffer.str("");
				buffer.clear();
				buffer << std::dec;

				spn::noseq_list<int> base, loaded;
				std::vector<decltype(base.add(0))> ids;
				for(int j=0 ; j<NITER ; j++) {
					// ランダムにデータを追加/削除する
					if(base.empty() || ((rd.getUniformRange<int>(0,3)) != 0)) {
						// 追加
						ids.push_back(base.add(rd.getUniform<int>()));
					} else {
						// 削除
						int idx = rd.getUniformRange<int>(0, ids.size()-1);
						auto itr = ids.begin() + idx;
						base.rem(*itr);
						ids.erase(itr);
					}
				}
				boost::archive::binary_oarchive oa(buffer);
				oa << base;
				boost::archive::binary_iarchive ia(buffer);
				ia >> loaded;
				Assert(Trap, base == loaded)
			}
		}
		void SerializeTest_resmgr() {
			auto rd = mgr_random.get(0);
			std::stringstream buffer;
			for(int i=0 ; i<NTEST ; i++) {
				Optional<TestRM> op;
				op = construct();
				std::vector<HLMy> handle;
				for(int j=0 ; j<NITER ; j++) {
					// ランダムにデータを追加・削除
					if(handle.empty() || rd.getUniformRange<int>(0, 1)==0) {
						// 追加
						handle.push_back(op->emplace(rd.getUniform<int>(), rd.getUniform<int>()));
					} else {
						// 削除
						int idx = rd.getUniformRange<int>(0, handle.size()-1);
						handle[idx].release();
						handle.erase(handle.begin()+idx);
					}
				}
				for(auto& p : handle)
					p.setNull();
				// 書き出し
				boost::archive::text_oarchive oa(buffer, boost::archive::no_header);
				// フル書き出しと差分と交互にチェックする
				if(i&1)
					oa << op->asMergeType();
				else
					oa << *op;

				// 比較元データを保管
				auto dat0 = op->getNSeq();

				// 一旦クリアしてから読み出し
				op = construct();
				boost::archive::text_iarchive ia(buffer, boost::archive::no_header);
				ia >> *op;
				auto dat1 = op->getNSeq();
				// ちゃんとデータがセーブできてるか確認
				Assert(Trap, dat0 == dat1)

				buffer.str("");
				buffer.clear();
				buffer << std::dec;
			}
		}
	}
}
