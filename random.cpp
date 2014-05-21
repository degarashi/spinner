#include "random.hpp"
#include "error.hpp"

namespace spn {
	namespace {
		constexpr int NUM_SEED = 32;
	}
	void MTRandomMgr::initEngine(ID entID) {
		std::random_device rnd;
		std::vector<Value>	seed(NUM_SEED);
		std::generate(seed.begin(), seed.end(), std::ref(rnd));
		initWithSeed(entID, seed);
	}
	void MTRandomMgr::initWithSeed(ID entID, const std::vector<Value>& rndv) {
		std::seed_seq seq(rndv.begin(), rndv.end());
		auto itr = _rndMap.find(entID);
		Assert(Trap, itr == _rndMap.end())
		_rndMap.emplace(entID, std::mt19937(seq));
	}
	MTRandom MTRandomMgr::operator()(ID entID) {
		return get(entID);
	}
	MTRandom MTRandomMgr::get(ID entID) {
		auto itr = _rndMap.find(entID);
		Assert(Trap, itr!=_rndMap.end(), "random generator %1% not found", entID)
		return MTRandom(itr->second);
	}
}
