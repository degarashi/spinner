/*! 演算クラスの実体化を行う */
#include "spn_math.hpp"

// 構造体の定義だけ
#include "vector.hpp"
#include "matrix.hpp"
#include "plane.hpp"
#include "quat.hpp"
#include "expquat.hpp"

// 他クラスとの演算
#define INCLUDE_MATRIX_LEVEL 1
#include "matrix.hpp"
