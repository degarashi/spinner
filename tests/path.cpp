#ifdef WIN32
	#include <intrin.h>
	namespace {
		const char* c_DriveLetter = "C:";
	}
#else
	namespace {
		const char* c_DriveLetter = "/";
	}
#endif
#include "test.hpp"
#include "../path.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace spn {
	namespace test {
		const std::basic_string<char> c_alphabet("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
		const std::basic_string<char> c_segment("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_ ");

		class PathTest : public RandomTestInitializer {};
		TEST_F(PathTest, Serialize) {
			auto rd = getRand();
			PathBlock pb = GenRPath(rd, {1,20}, {1,16}, c_DriveLetter);
			CheckSerializedDataBin(pb);
		}
	}
}

