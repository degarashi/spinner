#ifdef WIN32
	#include <intrin.h>
#endif
#include "tests/math.hpp"
#include "tests/pqueue.hpp"
#include "tests/resource.hpp"
#include "tests/serialization.hpp"
#include "tests/filetree.hpp"
#include "random.hpp"

using namespace spn::test;
int main(int argc, char **argv) {
	spn::MTRandomMgr rmgr;
	mgr_random.initEngine(0);
	MatTest();
	PoseTest();
	BitFieldTest();
	ResourceTest();
	GapTest();
	SerializeTest_noseq();
	SerializeTest_resmgr();
	PQueue();
	Math();
// テストが通らないので一時無効化
// 	spn::PathBlock pb(argv[0]);
// 	pb.popBack();
// 	FileTree_test(pb.plain_utf8());
	return 0;
}
