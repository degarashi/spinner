/*! 演算クラスの実体化を行う */
#include "spn_math.hpp"

// 構造体の定義だけ
#include "vector.hpp"
#include "matrix.hpp"
#include "plane.hpp"
#include "quat.hpp"
#include "expquat.hpp"

// 自身で完結する演算
#define INCLUDE_VECTOR_LEVEL 1
#include "vector.hpp"
#define INCLUDE_MATRIX_LEVEL 1
#include "matrix.hpp"
