#include "structure/profiler.hpp"

namespace spn {
	thread_local Profiler profiler;
	// -------------------- Profiler::Block --------------------
	Profiler::Block::Block(const Name& name, SerialId id):
		serialId(id),
		name(name),
		takeTime(0),
		nCalled(0)
	{}
	Profiler::USec Profiler::Block::getLowerTime() const {
		USec sum(0);
		iterateDepthFirst<false>([&sum](const Block& nd, int){
			sum += nd.takeTime;
			return Iterate::Next;
		});
		return sum;
	}
	// -------------------- Profiler::BlockObj --------------------
	Profiler::BlockObj::BlockObj(const Name& name):
		_bValid(true),
		_name(name)
	{}
	Profiler::BlockObj::BlockObj(BlockObj&& b):
		_bValid(true),
		_name(std::move(b._name))
	{
		b._bValid = false;
	}
	Profiler::BlockObj::~BlockObj() {
		profiler.endBlock(_name);
	}
	// -------------------- Profiler --------------------
	Profiler::Profiler() {
		reset();
	}
	void Profiler::reset() {
		_uniqueMap.clear();
		_singleMap.clear();
		_serialIdCur = 0;
		_currentBlock = nullptr;
		_spRoot.reset();

		// ルートノードを追加
		_makeBlock("Root");
	}
	Profiler::BlockSP Profiler::_makeBlock(const Name& name) {
		auto sp = std::make_shared<Block>(name, ++_serialIdCur);
		++sp->nCalled;

		SerialId id = _currentBlock ? _currentBlock->serialId : 0;
		auto key = std::make_pair(id, sp->name);
		Assert(Trap, _uniqueMap.count(key) == 0)
		_uniqueMap.emplace(key, sp);
		_singleMap[sp->name].emplace_back(sp.get());
		if(_currentBlock)
			_currentBlock->addChild(sp);
		else {
			Assert(Trap, !static_cast<bool>(_spRoot))
			_spRoot = sp;
		}
		_currentBlock = sp.get();
		return std::move(sp);
	}
	void Profiler::beginBlock(const Name& name) {
		_tmBegin = Clock::now();
		auto key = std::make_pair(_currentBlock->serialId, name);
		auto itr = _uniqueMap.find(key);
		if(itr != _uniqueMap.end()) {
			// 2度目以降の呼び出し
			_currentBlock = itr->second.get();
			++_currentBlock->nCalled;
		} else {
			// 初回呼び出し
			auto sp = _makeBlock(name);
			_currentBlock = sp.get();
		}
	}
	Profiler::BlockObj Profiler::beginBlockObj(const Name& name) {
		beginBlock(name);
		return BlockObj(name);
	}
	void Profiler::endBlock(const Name& name) {
		TimePoint tmEnd = Clock::now();
		// ネストコールが崩れた時にエラーを出す
		Assert(Trap, _currentBlock->name == name)
		// かかった時間を加算
		USec us = std::chrono::duration_cast<USec>(tmEnd - _tmBegin);
		_currentBlock->takeTime += us;
		// ポインタを親に移す
		_currentBlock = _currentBlock->getParent().get();
		// nullの場合、RootをPopしたという事なのでエラー
		Assert(Trap, _currentBlock)
	}
	std::pair<Profiler::USec,uint32_t> Profiler::getAccumedTime(const Name& name) const {
		uint32_t nc(0);
		USec us(0);
		auto& s = _singleMap.at(name);
		for(auto& b : s) {
			nc += b->nCalled;
			us += b->takeTime;
		}
		return std::make_pair(us, nc);
	}
	Profiler::BlockSP Profiler::getRoot() const {
		return _spRoot;
	}
}
