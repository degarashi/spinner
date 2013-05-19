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
	
	AMat33 m = {1,2,3,4,5,6,7,8,9};
	auto m2 = m.convert43();
	AMat23 m3 = {100,200,300,400,500,600};
	
    return 0;
}
