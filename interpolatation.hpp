#pragma once
#include <type_traits>

namespace spn {
	//! linear interpolation
	template <class T>
	T Lerp(const T& v0, const T& v1, float r) {
		return (v1-v0)*r + v0;
	}
	//! catmull-rom interpolation
	float CatmullRom(float v0, float v1, float v2, float v3, float t);
	template <int N, bool A>
	VecT<N,A> CatmullRom(const VecT<N,A>& v0, const VecT<N,A>& v1, const VecT<N,A>& v2, const VecT<N,A>& v3, float t) {
		VecT<N,A> ret;
		for(int i=0 ; i<N ; i++)
			ret.m[i] = CatmullRom(v0.m[i], v1.m[i], v2.m[i], v3.m[i], t);
		return ret;
	}
	/*! catmull-rom interpolation (when T0 is not available) */
	float CatmullRomT0(float v1, float v2, float v3, float t);
	/*! catmull-rom interpolation (when T3 is not available) */
	float CatmullRomT3(float v0, float v1, float v2, float t);
}

