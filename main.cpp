#include <iostream>
#include "vector.hpp"
#include "matrix.hpp"

using namespace spn;
int main(int argc, char **argv) {
	AMat33 mrX = AMat33::RotationX(DEGtoRAD(90));
	AVec3 vrot(0,0,100);
	auto vr = vrot * mrX;
	float len = vrot.length();
    return 0;
}
