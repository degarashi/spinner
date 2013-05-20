#include <iostream>
#include "spn_math.hpp"

using namespace spn;
int main(int argc, char **argv) {
	AVec3 v{1,0,0},
		v2{0,1,0};
	v %= v2;
	auto len = v.length();
	auto lensq = v.len_sq();
	v = AVec3(1,2,3);
	v.normalize();
	
	AMat34 m = {1,2,3,4,5,6,7,8,9,10,11,12},
			m2(10.f, AMat33::TagDiagonal);
	auto p = &m,
		p2 = &m2;
	auto m3 = m * m2;
	m *= m2;
    return 0;
}
