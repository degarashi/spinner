#include "structure/profiler.hpp"

namespace spn {
	thread_local prof::Profiler profiler;
	namespace prof {
		// -------------------- StdClock --------------------
		TimePoint StdClock::now() const {
			return Clock::now();
		}
		// -------------------- Profiler::Block --------------------
		Profiler::Block::Block(const Name& name):
			name(name)
		{}
		USec Profiler::Block::getLowerTime() const {
			USec sum(0);
			iterateChild([&sum](const auto& nd){
				sum += nd->hist.tAccum;
				return true;
			});
			return sum;
		}
		USec Profiler::Block::getAverageTime(const bool omitLower) const {
			if(omitLower)
				return (hist.tAccum - getLowerTime()) / hist.nCalled;
			return hist.getAverageTime();
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
		USec Profiler::History::getAverageTime() const {
			return tAccum / nCalled;
		}

		// -------------------- Profiler --------------------
		bool Profiler::_hasIntervalPassed() const {
			return (_clock->now() - getCurrent().tmBegin) >= _tInterval;
		}
		const int Profiler::DefaultMaxLayer = 8;
		const Duration Profiler::DefaultInterval = std::chrono::seconds(1);
		const Profiler::Name Profiler::DefaultRootName = "Root";
		Profiler::Profiler() {
			reInitialize();
		}
		void Profiler::clear() {
			_closeAllBlocks();
			_intervalInfo.clear();
			_intervalInfo.current().tmBegin = _clock->now();
			_curBlock = nullptr;
			_prepareRootNode();
		}
		void Profiler::_closeAllBlocks() {
			auto* cur = _curBlock;
			while(cur) {
				endBlock(cur->name);
				cur = cur->getParent().get();
			}
			_curBlock = nullptr;
		}
		void Profiler::_prepareRootNode() {
			beginBlock(_rootName);
		}
		bool Profiler::onFrame() {
			// ルート以外のブロックを全て閉じてない場合は警告を出す
			Assert(Warn, !accumulating(), "profiler block leaked")
			const bool bf = _hasIntervalPassed();
			_closeAllBlocks();
			// インターバル時間が過ぎていたら他にも変数をリセット
			if(bf) {
				_intervalInfo.advance_clear();
				_intervalInfo.current().tmBegin = _clock->now();
				_curBlock = nullptr;
			}
			_prepareRootNode();
			return bf;
		}
		void Profiler::reInitialize() {
			reInitialize(DefaultInterval);
		}
		void Profiler::_reInitialize(USec it, const Name& rootName, const int ml, ClockIF_UP c) {
			_rootName = rootName;
			// 計測中だった場合は警告を出す
			Assert(Warn, !accumulating(), "reinitializing when accumulating")
			Assert(Trap, c, "invalid clock")
			std::swap(_clock, c);
			Assert(Trap, ml > 0, "invalid maxLayer")
			_maxLayer = ml;
			_tInterval = it;
			clear();
		}
		bool Profiler::accumulating() const {
			// ルートノードの分を加味
			return _tmBegin.size() > 1;
		}
		void Profiler::beginBlock(const Name& name) {
			// 最大レイヤー深度を超えた分のブロック累積処理はしない
			const int n = _tmBegin.size();
			auto& ci = _intervalInfo.current();
			if(n < _maxLayer) {
				// 子ノードに同じ名前のノードが無いか確認
				auto& cur = _curBlock;
				Block::SP node;
				if(cur) {
					cur->iterateChild([&node, &name](auto& nd){
						if(nd->name == name) {
							node = nd->shared_from_this();
							return false;
						}
						return true;
					});
				}
				if(!node) {
					Assert(Trap, ci.root || n==0)
					if(!cur && ci.root && ci.root->name == name)
						cur = ci.root.get();
					else {
						// 新たにノードを作成
						const auto blk = std::make_shared<Block>(name);
						if(cur) {
							// 現在ノードの子に追加
							cur->addChild(blk);
						} else {
							// 新たにルートノードとして設定
							ci.root = blk;
						}
						cur = blk.get();
					}
				} else {
					cur = node.get();
				}
			}
			// エントリを確保
			ci.byName[name];
			// 現在のレイヤーの開始時刻を記録
			_tmBegin.emplace_back(_clock->now());
		}
		Profiler::BlockObj Profiler::beginBlockObj(const Name& name) {
			beginBlock(name);
			return BlockObj(name);
		}
		void Profiler::endBlock(const Name& name) {
			// endBlockの呼び過ぎはエラー
			Assert(Trap, !_tmBegin.empty())
			auto& cur = _curBlock;

			// かかった時間を加算
			const USec dur = std::chrono::duration_cast<USec>(_clock->now() - _tmBegin.back());
			{
				const int n = _tmBegin.size();
				if(n <= _maxLayer) {
					cur->hist.addTime(dur);
					// ネストコールが崩れた時にエラーを出す
					Assert(Trap, name==cur->name)
					// ポインタを親に移す
					cur = cur->getParent().get();
				}
			}
			auto& ci = _intervalInfo.current();
			Assert(Trap, ci.byName.count(name)==1)
			ci.byName.at(name).addTime(dur);
			_tmBegin.pop_back();
		}
		const Profiler::IntervalInfo& Profiler::getPrev() const {
			return _intervalInfo.prev();
		}
		const Profiler::IntervalInfo& Profiler::getCurrent() const {
			return _intervalInfo.current();
		}
		Profiler::BlockObj Profiler::operator()(const Name& name) {
			return beginBlockObj(name);
		}
	}
}
