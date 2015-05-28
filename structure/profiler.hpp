//! リアルタイムプロファイラ
#pragma once
#include "structure/treenode.hpp"
#include <chrono>
#include <unordered_map>

namespace spn {
	namespace {
		namespace prof {
			using SerialId = uint32_t;
			using Name = std::string;
			using UniquePair = std::pair<SerialId,Name>;
		}
	}
}
namespace std {
	template <>
	struct hash<spn::prof::UniquePair> {
		std::size_t operator()(const spn::prof::UniquePair& p) const {
			return hash<spn::prof::UniquePair::first_type>()(p.first) ^
					hash<spn::prof::UniquePair::second_type>()(p.second);
		}
	};
}
namespace spn {
	// スレッド毎に集計(=結果をスレッド毎に取り出す)
	class Profiler {
		public:
			using Clock = std::chrono::steady_clock;
			using TimePoint = Clock::time_point;
			using Name = prof::Name;
			using USec = std::chrono::microseconds;
		private:
			using SerialId = prof::SerialId;
			//! プロファイラブロック
			struct Block : spn::TreeNode<Block> {
				SerialId		serialId;	//!< ツリー全体を通しての通し番号
				Name			name;		//!< ブロック名
				USec			takeTime;	//!< 掛かった時間 (下層を含む)
				uint32_t		nCalled;	//!< 呼び出された回数
				USec			getLowerTime() const;

				Block(const Name& name, SerialId id);
			};
			using BlockSP = std::shared_ptr<Block>;
			// key = 親ノードのSerialId +'|'+ nameがキー
			using UniqueMap = std::unordered_map<prof::UniquePair, BlockSP>;
			// key = ノード名
			using SingleMap = std::unordered_map<Name, std::vector<Block*>>;
			UniqueMap	_uniqueMap;		//!< ブロックのユニーク名マップ
			SingleMap	_singleMap;		//!< 同じ名前のブロック配列
			SerialId	_serialIdCur;

			BlockSP		_spRoot;
			//! 現在計測中のブロック
			Block*		_currentBlock;
			//! 計測開始した時刻
			TimePoint	_tmBegin;

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
			BlockSP _makeBlock(const Name& name);
		public:
			Profiler();
			//! 内部情報をリセットして次の計測に備える
			void reset();
			void beginBlock(const Name& name);
			void endBlock(const Name& name);
			BlockObj beginBlockObj(const Name& name);
			// 同じ名前のブロックを合算
			std::pair<USec,uint32_t> getAccumedTime(const Name& name) const;
			// ツリー構造ルート取得
			BlockSP getRoot() const;
	};
	extern thread_local Profiler profiler;
	namespace prof {}
}
