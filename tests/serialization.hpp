#pragma once
#include "../resmgr.hpp"
#include <boost/serialization/access.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace spn {
	namespace test {
		class TestRM;
		struct MyEnt;
	}
	DEF_AHANDLE(test::TestRM, My, test::MyEnt, test::MyEnt)

	namespace test {
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
	}
	namespace test {
		constexpr static int NTEST = 256,
							NITER = 1024;
		void SerializeTest_noseq();
		void SerializeTest_resmgr();
	}
}
