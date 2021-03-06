/*! 演算クラスの実体化を行う */
#include "spn_math.hpp"

// 構造体の定義だけ
#include "vector.hpp"
#include "matrix.hpp"
#include "plane.hpp"
#include "quat.hpp"
#include "expquat.hpp"

// 他クラスとの演算
#define INCLUDE_VECTOR_LEVEL 2
#include "vector.hpp"
#define INCLUDE_VECTOR_LEVEL 3
#include "vector.hpp"
#define INCLUDE_PLANE_LEVEL 1
#include "plane.hpp"
#define INCLUDE_MATRIX_LEVEL 2
#include "matrix.hpp"
#define INCLUDE_QUAT_LEVEL 1
#include "quat.hpp"
#define INCLUDE_EXPQUAT_LEVEL 1
#include "expquat.hpp"
