#include "test.hpp"

namespace spn {
	namespace test {
		Random::Random() {
			std::random_device rdev;
			std::array<uint_least32_t, 32> seed;
			std::generate(seed.begin(), seed.end(), std::ref(rdev));
			std::seed_seq seq(seed.begin(), seed.end());
			_opMt = std::mt19937(seq);
			_opDistF = std::uniform_real_distribution<float>{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()};
			_opDistI = std::uniform_int_distribution<int>{std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max()};
		}
		float Random::randomF() {
			return (*_opDistF)(*_opMt);
		}
		int Random::randomI() {
			return (*_opDistI)(*_opMt);
		}
		int Random::randomI(int from, int to) {
			return std::uniform_int_distribution<int>{from, to}(*_opMt);
		}
		int Random::randomIPositive(int to) {
			return std::uniform_int_distribution<int>{0, to}(*_opMt);
		}
		Random& Random::getInstance() {
			static Random rd;
			return rd;
		}
	}
}
