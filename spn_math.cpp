/*! 演算クラスの実体化を行う */
#include "spn_math.hpp"
#include <cstring>
namespace spn {
	reg128
		xmm_const::_tmp0001,
		xmm_const::_tmp0001_i,
		xmm_const::_tmp0111,
		xmm_const::_tmp0111_i,
		xmm_const::_tmp0000,
		xmm_const::_tmp0000_i,
		xmm_const::_tmp1000,
		xmm_const::_tmp1000_i,
		xmm_const::_tmp1111,
		xmm_const::_tmp1111_i,
		xmm_const::_epsilon,
		xmm_const::_epsilonM,
		xmm_const::_minus0,
		xmm_const::_fullbit,
		xmm_const::_absmask;
	reg128
		xmm_const::_mask[4],
		xmm_const::_maskN[4],
		xmm_const::_matI[4];
	float
		xmm_const::_cs_matI[4][4];

	void xmm_const::Initialize() {
		_tmp0001 = _mmSetPs(1,0,0,0); _tmp0001_i = _mmSetPdw(-1,0,0,0);
		_tmp0111 = _mmSetPs(1,1,1,0); _tmp0111_i = _mmSetPdw(-1,-1,-1,0);
		_tmp0000 = _mmSetPs(0,0,0,0); _tmp0000_i = _mmSetPdw(0,0,0,0);
		_tmp1000 = _mmSetPs(0,0,0,1); _tmp1000_i = _mmSetPdw(0,0,0,-1);
		_tmp1111 = _mmSetPs(1,1,1,1); _tmp1111_i = _mmSetPdw(-1);
		_epsilon = _mmSetPs(FLOAT_EPSILON);
		_epsilonM = _mmSetPs(-FLOAT_EPSILON);
		_minus0 = _mmSetPs(-0.f, -0.f, -0.f, -0.f);
		_fullbit = reg_load1_ps(reinterpret_cast<const float*>(&Fullbit));
		_absmask = reg_load1_ps(reinterpret_cast<const float*>(&Absbit));

		_mask[0] = _makeMask<1,0,0,0>();
		_mask[1] = _makeMask<1,1,0,0>();
		_mask[2] = _makeMask<1,1,1,0>();
		_mask[3] = _makeMask<1,1,1,1>();
		_maskN[0] = _makeMask<0,1,1,1>();
		_maskN[1] = _makeMask<0,0,1,1>();
		_maskN[2] = _makeMask<0,0,0,1>();
		_maskN[3] = _makeMask<0,0,0,0>();
		const float tmp[4][4] = {
			{1,0,0,0},
			{0,1,0,0},
			{0,0,1,0},
			{0,0,0,1}
		};
		std::memcpy(const_cast<float*>(&_cs_matI[0][0]), tmp, sizeof(tmp));
		_matI[0] = _mmSetPs(1,0,0,0);
		_matI[1] = _mmSetPs(0,1,0,0);
		_matI[2] = _mmSetPs(0,0,1,0);
		_matI[3] = _mmSetPs(0,0,0,1);
	}
	void xmm_const::Terminate() {}
}

// 構造体の定義だけ
#include "vector.hpp"
#include "matrix.hpp"
#include "plane.hpp"
#include "quat.hpp"
#include "expquat.hpp"

// 自身で完結する演算
#define INCLUDE_VECTOR_LEVEL 1
#include "vector.hpp"
