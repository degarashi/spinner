#include "structure/profiler.hpp"

namespace spn {
	// -------------------- Profiler::Block --------------------
	Profiler::Block::Block(const Name& name):
		name(name),
		layerHistId(0)
	{}
	Profiler::USec Profiler::Block::getLowerTime() const {
		USec sum(0);
		iterateDepthFirst<false>([&sum](const Block& nd, int depth){
			if(depth > 0) {
				sum += nd.hist.tAccum;
				return Iterate::Next;
			} else
				return Iterate::StepIn;
		});
		return sum;
	}
	// -------------------- Profiler::BlockObj --------------------
	Profiler::BlockObj::BlockObj(const Name& name):
		_bValid(true),
		_name(name)
	{}
	Profiler::BlockObj::BlockObj(BlockObj&& b):
		_bValid(b._bValid),
		_name(std::move(b._name))
	{
		b._bValid = false;
	}
	Profiler::BlockObj::~BlockObj() {
		if(_bValid)
			profiler.endBlock(_name);
	}
	// -------------------- Profiler::History --------------------
	Profiler::History::History():
		nCalled(0),
		tMax(std::numeric_limits<USec::rep>::lowest()),
		tMin(std::numeric_limits<USec::rep>::max()),
		tAccum(0)
	{}
	void Profiler::History::addTime(USec t) {
		++nCalled;
		tMax = std::max(tMax, t);
		tMin = std::min(tMin, t);
		tAccum += t;
	}
	Profiler::USec Profiler::History::getAverageTime() const {
		return tAccum / nCalled;
	}

	thread_local Profiler profiler;
	namespace {
		const char* c_rootName = "Root";
	}
	// -------------------- Profiler --------------------
	bool Profiler::_hasIntervalPassed() const {
		return (Clock::now() - _tIntervalCur) >= _tInterval;
	}
	Profiler::Profiler() {
		clear();
	}
	void Profiler::clear() {
		_intervalInfo.clear();
		_spRoot.clear();
		while(!_tmBegin.empty())
			_tmBegin.pop();
		_current.pBlock = nullptr;
		_current.histId = 0;
		setInterval(std::chrono::seconds(1));

		beginBlock(c_rootName);
	}
	void Profiler::resetTree() {
		auto* cur = _current.pBlock;
		while(cur) {
			endBlock(cur->name);
			cur = cur->getParent().get();
			_current.pBlock = cur;
		}
		while(!_tmBegin.empty())
			_tmBegin.pop();
		_spRoot.advance_clear();

		// インターバル時間が過ぎていたら他にも変数をリセット
		if(_hasIntervalPassed()) {
			_tIntervalCur = Clock::now();
			_intervalInfo.advance_clear();
		}
		// ルートノードを追加
		beginBlock("Root");
	}
	void Profiler::beginBlock(const Name& name) {
		int n = _tmBegin.size();
		auto& ci = _intervalInfo.current();
		if(n < MaxLayer) {
			// 現在のレイヤーにおける名前Idを取得
			int nameId;
			{
				auto& nv = ci.nameV[n];
				auto itr = std::find(nv.begin(), nv.end(), name);
				if(itr == nv.end()) {
					nameId = nv.size() + 1;
					nv.emplace_back(name);
				} else
					nameId = (itr - nv.begin()) + 1;
			}
			_current.histId |= CalcLayerHistId(n, nameId);
		}
		{
			// 既に同じノードが無いか確認
			auto& cur = _current.pBlock;
			Block::SP node;
			if(cur) {
				cur->iterateDepthFirst<false>([&node, &name](Block& nd, int depth){
					if(depth == 0)
						return Iterate::StepIn;
					if(nd.name == name) {
						node = nd.shared_from_this();
						return Iterate::Quit;
					}
					return Iterate::Next;
				});
			}
			if(!node) {
				auto blk = std::make_shared<Block>(name);
				blk->layerHistId = (n<MaxLayer) ? _current.histId : 0;
				ci.nameHistMap[name];
				if(cur)
					cur->addChild(blk);
				else {
					Assert(Trap, n==0)
					Assert(Trap, !_spRoot.current())
					_spRoot.current() = blk;
				}
				cur = blk.get();
			} else {
				cur = node.get();
			}
		}
		_tmBegin.push(Clock::now());
	}
	Profiler::BlockObj Profiler::beginBlockObj(const Name& name) {
		beginBlock(name);
		return BlockObj(name);
	}
	void Profiler::endBlock(const Name& name) {
		auto& cur = _current.pBlock;
		// ネストコールが崩れた時にエラーを出す
		Assert(Trap, name==cur->name)

		int n = _tmBegin.size();
		// かかった時間を加算
		USec dur = std::chrono::duration_cast<USec>(Clock::now() - _tmBegin.top());

		auto& ci = _intervalInfo.current();
		if(n <= MaxLayer) {
			// 名前Id履歴からブロックを検索 & 用意
			auto& h = ci.intervalHistMap[_current.histId];
			h.addTime(dur);
			// 名前Idの先頭を00で埋める
			ClearLayerHistId(_current.histId, n-1);
		}
		Assert(Trap, ci.nameHistMap.count(name)==1)
		ci.nameHistMap.at(name).addTime(dur);
		cur->hist.addTime(dur);
		// ポインタを親に移す
		cur = cur->getParent().get();
		_tmBegin.pop();
	}
	Profiler::LayerHistId Profiler::CalcLayerHistId(int level, int index) {
		return LayerHistId(index & 0xff) << ((sizeof(LayerHistId) - level - 1)*8);
	}
	void Profiler::ClearLayerHistId(LayerHistId& id, int level) {
		id &= ~CalcLayerHistId(level, 0xff);
	}
	Profiler::BlockSP Profiler::getRoot() const {
		return _spRoot.prev();
	}
	const Profiler::NameHistMap& Profiler::getNameHistory() const {
		return _intervalInfo.prev().nameHistMap;
	}
	const Profiler::IntervalHistMap& Profiler::getIntervalHistory() const {
		return _intervalInfo.prev().intervalHistMap;
	}
}
