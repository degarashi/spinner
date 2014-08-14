#include "test.hpp"
#include "../resmgr.hpp"
#include "../pose.hpp"
#include <boost/serialization/access.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace spn {
	namespace test {
		namespace {
			constexpr static int NTEST = 16,
								NITER = 1024;
			class TestRM;
			struct MyEnt;
			DEF_AHANDLE(test::TestRM, My, test::MyEnt, test::MyEnt)

			// デフォルトコンストラクタ無し、コピー不可能なテスト用構造体
			struct MyEnt : boost::serialization::traits<MyEnt,
						boost::serialization::object_serializable,
						boost::serialization::track_never>
			{
				int value0,
					value1;
				MyEnt() = default;
				MyEnt(const MyEnt&) = delete;
				MyEnt(MyEnt&& e): value0(e.value0), value1(e.value1) {}
				void operator = (const MyEnt&) = delete;

				template <class Archive>
				void serialize(Archive& ar, const unsigned int) {
					ar & value0 & value1;
				}
				MyEnt(int v0, int v1): value0(v0), value1(v1) {}
				bool operator == (const MyEnt& e) const {
					return value0 == e.value0 &&
							value1 == e.value1;
				}
			};
			class TestRM : public ResMgrA<MyEnt, TestRM> {};
			class SerializeTest : public RandomTestInitializer {};
		}

		template <class T>
		class SerializeVector : public RandomTestInitializer {
			protected:
				using base_t = RandomTestInitializer;
		};
		TYPED_TEST_CASE(SerializeVector, VecTP);
		TYPED_TEST(SerializeVector, Test) {
			using This_t = std::decay_t<decltype(*this)>;
			auto rd = This_t::base_t::getRand();
			auto rdF = [&rd](){ return rd.template getUniformRange<float>(-1e3f, 1e3f); };
			for(int i=0 ; i<NTEST ; i++) {
				auto data = GenRVec<TypeParam::width, TypeParam::align>(rdF);
				CheckSerializedDataBin(data);
			}
		}
		

		template <class T>
		using SerializeMatrix = SerializeVector<T>;
		TYPED_TEST_CASE(SerializeMatrix, MatTP);
		TYPED_TEST(SerializeMatrix, Test) {
			using This_t = std::decay_t<decltype(*this)>;
			auto rd = This_t::base_t::getRand();
			auto rdF = [&rd](){ return rd.template getUniformRange<float>(-1e3f, 1e3f); };
			for(int i=0 ; i<NTEST ; i++) {
				auto data = GenRMat<TypeParam::align, TypeParam::height, TypeParam::width>(rdF);
				CheckSerializedDataBin(data);
			}
		}

		using SerializePose2D = RandomTestInitializer;
		TEST_F(SerializePose2D, Test) {
			using This_t = std::decay_t<decltype(*this)>;
			auto rd = This_t::getRand();
			auto rdF = [&rd](){ return rd.template getUniformRange<float>(-1e3f, 1e3f); };
			for(int i=0 ; i<NTEST ; i++) {
				Pose2D ps(GenRVec<2,false>(rdF),
							rdF(),
							GenRVec<2,false>(rdF));
				CheckSerializedDataBin(ps);
			}
		}
		using SerializePose3D = RandomTestInitializer;
		TEST_F(SerializePose3D, Test) {
			using This_t = std::decay_t<decltype(*this)>;
			auto rd = This_t::getRand();
			auto rdF = [&rd](){ return rd.template getUniformRange<float>(-1e3f, 1e3f); };
			for(int i=0 ; i<NTEST ; i++) {
				Pose3D ps(GenRVec<3,true>(rdF),
							GenRQuat<true>(rdF),
							GenRVec<3,true>(rdF));
				CheckSerializedDataBin(ps);
			}
		}

		TEST_F(SerializeTest, Noseq) {
			auto rd = getRand();
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
				EXPECT_EQ(base, loaded);
			}
		}
		TEST_F(SerializeTest, ResMgr) {
			auto rd = getRand();
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
				EXPECT_EQ(dat0, dat1);

				buffer.str("");
				buffer.clear();
				buffer << std::dec;
			}
		}
	}
}
