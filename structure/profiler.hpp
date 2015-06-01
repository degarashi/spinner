//! リアルタイムプロファイラ
#pragma once
#include "structure/treenode.hpp"
#include "structure/dataswitch.hpp"
#include <chrono>
#include <unordered_map>
#include <stack>

namespace spn {
	// スレッド毎に集計(=結果をスレッド毎に取り出す)
	class Profiler {
		private:
			//! ツリー構造をたどった履歴
			using LayerHistId = uint64_t;
		public:
			using Name = const char*;
			using Clock = std::chrono::steady_clock;
			using TimePoint = Clock::time_point;
			using USec = std::chrono::microseconds;
			struct History {
				uint32_t		nCalled;	//!< 呼び出し回数
				USec			tMax,		//!< 最高値
								tMin,		//!< 最低値
								tAccum;		//!< 累積値
				History();
				void addTime(USec t);
				USec getAverageTime() const;
			};
			//! プロファイラブロック
			struct Block : spn::TreeNode<Block> {
				Name			name;				//!< ブロック名
				History			hist;				//!< 1フレーム間の履歴
				LayerHistId		layerHistId;

				Block(const Name& name);
				USec getLowerTime() const;			//!< 下層にかかった時間
			};
			using BlockSP = std::shared_ptr<Block>;
		private:
			constexpr static int MaxLayer = 8;
			using IntervalHistMap = std::unordered_map<LayerHistId, History>;
			using NameV = std::vector<Name>;
			using NameHistMap = std::unordered_map<Name, History>;
			using TPStk = std::stack<TimePoint>;

			struct IntervalInfo {
				IntervalHistMap	intervalHistMap;		//!< 1インターバル間の履歴
				NameHistMap		nameHistMap;			//!< 同一名前ブロックの履歴(1インターバル有効)
				NameV			nameV[MaxLayer];		//!< 階層ごとの名前リスト
			};
			using IntervalInfoSW = DataSwitcher<IntervalInfo>;
			IntervalInfoSW	_intervalInfo;

			using BlockSPSW = DataSwitcher<BlockSP>;
			BlockSPSW		_spRoot;				//!< ツリー構造ルート

			TPStk			_tmBegin;				//!< 計測開始した時刻
			USec			_tInterval;				//!< 履歴を更新する間隔
			TimePoint		_tIntervalCur;
			bool _hasIntervalPassed() const;

			struct {
				Block*			pBlock;			//!< 現在計測中のブロック
				LayerHistId		histId;			//!< ツリーをたどった履歴(LayerIdを上位ビットから詰める)
			} _current;
			static LayerHistId CalcLayerHistId(int level, int index);
			static void ClearLayerHistId(LayerHistId& id, int level);

			//! スコープを抜けると同時にブロックを閉じる
			class BlockObj {
				private:
					bool	_bValid;
					Name	_name;
					friend class Profiler;
					BlockObj(const Name& name);
				public:
					BlockObj(BlockObj&& b);
					~BlockObj();
			};
		public:
			Profiler();
			template <class T>
			void setInterval(T t) {
				_tInterval = std::chrono::duration_cast<USec>(t);
				_tIntervalCur = Clock::now();
			}
			//! 内部情報をリセットして次の計測に備える
			void resetTree();
			void clear();
			void beginBlock(const Name& name);
			void endBlock(const Name& name);
			BlockObj beginBlockObj(const Name& name);
			// 同じ名前のブロックを合算したものを取得
			const NameHistMap& getNameHistory() const;
			const IntervalHistMap& getIntervalHistory() const;
			// ツリー構造ルート取得
			BlockSP getRoot() const;
	};
	extern thread_local Profiler profiler;
	namespace prof {}
}
