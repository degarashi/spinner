#include <iostream>
#include "vector.hpp"

int main(int argc, char **argv) {
	AVec3 v{1,0,0},
		v2{0,1,0};
	v %= v2;
	auto len = v.length();
	auto lensq = v.len_sq();
	v = AVec3(1,2,3);
	v.normalize();
    return 0;
}
