#include "tests/test.hpp"

int main(int argc, char **argv) {
	spn::test::g_apppath.setPath(argv[0]);
	spn::test::g_apppath.popBack();

	spn::MTRandomMgr rmgr;
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
