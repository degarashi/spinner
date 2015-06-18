#include "interpolatation.hpp"
#include "matrix.hpp"

namespace spn {
	namespace {
		const AMat44
			c_mCoeff = AMat44(
				-1.0f,	3.0f,	-3.0f,	1.0f,
				2.0f,	-5.0f,	4.0f,	-1.0f,
				-1.0f,	0.0f,	1.0f,	0.0f,
				0.0f,	2.0f,	0.0f,	0.0f
			) * 0.5f,
			c_mCoeffT0 = AMat44(
				0,		0,		0,		0,
				0,		1.0f,	-2.0f,	1.0f,
				0,		-3.0f,	4.0f,	-1.0f,
				0,		2.0f,	0,		0
			) * 0.5f,
			c_mCoeffT3 = AMat44(
				0,		0,		0,		0,
				1.0f,	-2.0f,	1.0f,	0,
				-1.0f,	0,		1.0f,	0,
				0,		2.0f,	0,		0
			) * 0.5f;
		float _CatmullRom(const AMat44& coeff, float v0, float v1, float v2, float v3, float t) {
			AVec4	vt(Cubic(t), Square(t), t, 1);
			vt *= coeff;
			return v0 * vt.x + v1 * vt.y + v2 * vt.z + v3 * vt.w;
		}
	}
	float CatmullRom(float v0, float v1, float v2, float v3, float t) {
		return _CatmullRom(c_mCoeff, v0, v1, v2, v3, t);
	}
	float CatmullRomT0(float v1, float v2, float v3, float t) {
		return _CatmullRom(c_mCoeffT0, 0, v1, v2, v3, t);
	}
	float CatmullRomT3(float v0, float v1, float v2, float t) {
		return _CatmullRom(c_mCoeffT3, v0, v1, v2, 0, t);
	}
}

