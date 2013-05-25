#include <iostream>
#include "vector.hpp"
#include "matrix.hpp"
#include "matstack.hpp"
#include "pose.hpp"

using namespace spn;
int main(int argc, char **argv) {
	AMat33 mrX = AMat33::RotationX(DEGtoRAD(90));
	AVec3 vrot(0,0,100);
	auto vr = vrot * mrX;
	float len = vrot.length();
	
	using MyStack = MatStack<Mat33, MatStackTag::PushRight>;
	MyStack stk;
	stk.push(Mat33(mrX));
	
	Pose2D p2(Vec2(100,200), 128, Vec2(1,1)),
			p3(Vec2(1,2), 64, Vec2(3,3));
	auto p4 = p2.lerp(p3, 0.5f);
    return 0;
}
