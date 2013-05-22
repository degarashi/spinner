/*! 演算クラスの実体化を行う */
#include "spn_math.hpp"

// 構造体の定義だけ
#include "vector.hpp"
#include "matrix.hpp"

// 自身で完結する演算
#define INCLUDE_LEVEL 1
#include "vector.hpp"
#define INCLUDE_LEVEL 1
#include "matrix.hpp"

// 他クラスとの演算
#define INCLUDE_LEVEL 2
#include "vector.hpp"
#define INCLUDE_LEVEL 3
#include "vector.hpp"
#define INCLUDE_LEVEL 2
#include "matrix.hpp"
