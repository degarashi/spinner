#include "tests/math.hpp"
#include "tests/pqueue.hpp"
#include "tests/resource.hpp"
#include "tests/serialization.hpp"

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
	return 0;
}
