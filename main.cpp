#include <iostream>
#include "vector.hpp"
#include "matrix.hpp"
#include "matstack.hpp"
#include "pose.hpp"
#include "misc.hpp"
#include "noseq.hpp"

using namespace spn;
int main(int argc, char **argv) {
	Mat44 m4 = Mat44::RotationZ(DEGtoRAD(0));
	auto idx = Mat44(Mat44::TagIdentity);
	auto mm4 = m4 * idx;
	m4 *= idx;

	AMat43 mrY = AMat43::RotationY(DEGtoRAD(45));
	auto mrY4 = mrY.convertA44();

	AMat33 mrX = AMat33::RotationX(DEGtoRAD(90));
	AVec3 vrot(0,0,100);
	auto vr = vrot * mrX;
	float len = vrot.length();

	using MyStack = MatStack<Mat33, MatStackTag::PushRight>;
	MyStack stk;
	stk.push(Mat33(mrX));

	Pose2D p2(AVec2(100,200), 128, AVec2(1,1)),
			p3(AVec2(1,2), 64, AVec2(3,3));
	auto p4 = p2.lerp(p3, 0.5f);

	auto pc = AVec3::FromPacked(MakeChunk(64,128,192,255));
	auto pc2 = pc.toPacked();

	noseq_list<float> ls;
	auto id0 = ls.add(256);
	auto id1 = ls.add(512);
	auto id2 = ls.add(1024);
	ls.rem(id1);
	for(auto& p : ls) {
		std::cout << p << std::endl;
	}
	return 0;
}
