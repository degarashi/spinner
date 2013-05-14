#include <iostream>
#include "vector.hpp"

int main(int argc, char **argv) {
	AVec4 v{1,2,3,4},
		v2{100,200,300,400};
	v += v2;
    return 0;
}
