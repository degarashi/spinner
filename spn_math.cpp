/*! 演算クラスの実体化を行う */
#include "spn_math.hpp"
namespace spn {
	const uint32_t fullbit = 0xffffffff,
						absbit = 0x7fffffff;
	const reg128 xmm_tmp0001(_mmSetPs(1,0,0,0)), xmm_tmp0001_i(_mmSetPdw(-1,0,0,0)),
				xmm_tmp0111(_mmSetPs(1,1,1,0)), xmm_tmp0111_i(_mmSetPdw(-1,-1,-1,0)),
				xmm_tmp0000(_mmSetPs(0,0,0,0)), xmm_tmp0000_i(_mmSetPdw(0,0,0,0)),
				xmm_tmp1000(_mmSetPs(0,0,0,1)), xmm_tmp1000_i(_mmSetPdw(0,0,0,-1)),
				xmm_tmp1111(_mmSetPs(1,1,1,1)), xmm_tmp1111_i(_mmSetPdw(-1)),
				xmm_epsilon(_mmSetPs(FLOAT_EPSILON)),
				xmm_epsilonM(_mmSetPs(-FLOAT_EPSILON)),
				xmm_minus0(_mmSetPs(-0.f, -0.f, -0.f, -0.f)),
				xmm_fullbit(reg_load1_ps(reinterpret_cast<const float*>(&fullbit))),
				xmm_absmask(reg_load1_ps(reinterpret_cast<const float*>(&absbit)));
	const reg128 xmm_mask[4] = {_makeMask<1,0,0,0>(),
										_makeMask<1,1,0,0>(),
										_makeMask<1,1,1,0>(),
										_makeMask<1,1,1,1>()},
				xmm_maskN[4] = {_makeMask<0,1,1,1>(),
								_makeMask<0,0,1,1>(),
								_makeMask<0,0,0,1>(),
								_makeMask<0,0,0,0>()};
	const float cs_matI[4][4] = {
		{1,0,0,0},
		{0,1,0,0},
		{0,0,1,0},
		{0,0,0,1}
	};
	const reg128 xmm_matI[4] = {
		_mmSetPs(1,0,0,0),
		_mmSetPs(0,1,0,0),
		_mmSetPs(0,0,1,0),
		_mmSetPs(0,0,0,1)
	};
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
