#include <intrin.h>
#include "tests/math.hpp"
#include "tests/pqueue.hpp"
#include "tests/resource.hpp"
#include "tests/serialization.hpp"
#include "tests/filetree.hpp"

using namespace spn::test;
int main(int argc, char **argv) {
	MatTest();
	PoseTest();
	BitFieldTest();
	ResourceTest();
	GapTest();
	SerializeTest_noseq();
	SerializeTest_resmgr();
	PQueue();
	Math();
	spn::PathBlock pb(argv[0]);
	pb.popBack();
	FileTree_test(pb.plain_utf8());
	return 0;
}
