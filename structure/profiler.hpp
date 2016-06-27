//! リアルタイムプロファイラ
#pragma once
#include "structure/treenode.hpp"
#include "structure/dataswitch.hpp"
#include <chrono>
#include <unordered_map>
#include <stack>

namespace spn {
	namespace prof {
		using Clock = std::chrono::steady_clock;
		using TimePoint = Clock::time_point;
		using Duration = Clock::duration;
		using USec = std::chrono::microseconds;
		struct ClockIF {
			virtual TimePoint now() const = 0;
		};
		struct StdClock : ClockIF {
			TimePoint now() const override;
		};
		using ClockIF_UP = std::unique_ptr<ClockIF>;

		// スレッド毎に集計(=結果をスレッド毎に取り出す)
		class Profiler {
			public:
				using Name = const char*;
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
					History			hist;

					Block(const Name& name);
					USec getLowerTime() const;			//!< 下層にかかった時間
				};
				using BlockSP = std::shared_ptr<Block>;
			private:
				//! 1インターバル間に集計される情報
				struct IntervalInfo {
					TimePoint	tmBegin;
					BlockSP		root;				//!< ツリー構造ルート
					using ByName = std::unordered_map<Name, History>;
					ByName		byName;				//!< 名前ブロック毎の集計(最大レイヤー数を超えた分も含める)
				};
				using IntervalInfoSW = DataSwitcher<IntervalInfo>;
				IntervalInfoSW	_intervalInfo;

				Name			_rootName;
				ClockIF_UP		_clock;
				USec			_tInterval;			//!< 履歴を更新する間隔
				Block*			_curBlock;			//!< 現在計測中のブロック
				int				_maxLayer;			//!< 最大ツリー深度
				using TPStk = std::vector<TimePoint>;
				TPStk			_tmBegin;			//!< レイヤー毎の計測開始した時刻
				bool _hasIntervalPassed() const;
				//! 閉じられていないブロックを閉じる(ルートノード以外)
				void _closeAllBlocks();
				void _reInitialize(USec it, const Name& rootName, int ml, ClockIF_UP c);
				void _prepareRootNode();

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
				const static Name DefaultRootName;
				const static int DefaultMaxLayer;
				const static Duration DefaultInterval;

				Profiler();
				//! 任意のインターバルとレイヤー数で再初期化
				template <class T>
				void reInitialize(
						T t,
						const Name& rootName = DefaultRootName,
						const int ml=DefaultMaxLayer,
						ClockIF_UP c=ClockIF_UP(new StdClock())
				) {
					_reInitialize(std::chrono::duration_cast<USec>(t), rootName, ml, std::move(c));
				}
				void reInitialize();
				//! 何かしらのブロックを開いているか = 計測途中か
				bool accumulating() const;
				//! フレーム間の区切りに呼ぶ
				/*! 必要に応じてインターバル切り替え */
				bool onFrame();
				void clear();
				void beginBlock(const Name& name);
				void endBlock(const Name& name);
				BlockObj beginBlockObj(const Name& name);
				BlockObj operator()(const Name& name);
				//! 同じ名前のブロックを合算したものを取得(前のインターバル)
				const IntervalInfo& getPrev() const;
				const IntervalInfo& getCurrent() const;
		};
	}
	extern thread_local prof::Profiler profiler;
}
