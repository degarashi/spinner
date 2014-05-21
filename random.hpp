#pragma once
#include <unordered_map>
#include <random>
#include <limits>
#include "misc.hpp"

namespace spn {
	//! Mersenne Twister(32bit)を用いた乱数生成
	class MTRandom {
		private:
			template <class T>
			struct Dist_Int {
				using uniform_t = std::uniform_int_distribution<T>;
			};
			template <class T>
			struct Dist_Float  {
				using uniform_t = std::uniform_real_distribution<T>;
			};

			template <class T, class=typename std::enable_if<std::is_integral<T>::value>::type>
			static Dist_Int<T> DetectDist();
			template <class T, class=typename std::enable_if<std::is_floating_point<T>::value>::type>
			static Dist_Float<T> DetectDist();
			template <class T>
			using GetDist = decltype(DetectDist<T>());
			template <class T>
			using L = std::numeric_limits<T>;

			std::mt19937&	_mt;
		public:
			MTRandom(std::mt19937& mt): _mt(mt) {}
			
			//! 一様分布
			/*! floating-pointの場合は0から1の乱数
				integerの場合は範囲指定なしnumeric_limits<T>::min() -> max()の乱数 */
			template <class T>
			T getUniform() {
				return getUniformRange(L<T>::lowest(), L<T>::max());
			}
			//! 指定範囲の一様分布(vmax)
			template <class T>
			T getUniformMax(const T& vmax) {
				return getUniformRange(L<T>::lowest(), vmax);
			}
			//! 指定範囲の一様分布(vmin)
			template <class T>
			T getUniformMin(const T& vmin) {
				return getUniformRange(vmin, L<T>::max());
			}
			//! 指定範囲の一様分布(vmin -> vmax)
			template <class T>
			T getUniformRange(const T& vmin, const T& vmax) {
				return typename GetDist<T>::uniform_t(vmin, vmax)(_mt);
			}
	};
	#define mgr_random (::spn::MTRandomMgr::_ref())
	//! MTRandomマネージャ
	class MTRandomMgr : public Singleton<MTRandomMgr> {
		private:
			using ID = int32_t;
			using Value = std::mt19937::result_type;
			using RndMap = std::unordered_map<ID, std::mt19937>;
			RndMap	_rndMap;

		public:
			//! ランダムにシードを初期化
			/*! 重複しての初期化はNG */
			void initEngine(ID entID);
			//! シードを指定して初期化
			void initWithSeed(ID entID, const std::vector<Value>& rndv);
			void removeEngine(ID entID);
			//! ランダム生成器を取り出す
			/*! 初期化せずに呼び出すと例外を投げる */
			MTRandom operator()(ID entID);
			MTRandom get(ID entID);
	};
}
