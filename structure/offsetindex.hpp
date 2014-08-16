#pragma once
#include <vector>

namespace spn {
	//! グループ分けされた値を一本のvectorに収める
	template <class T>
	class OffsetIndex {
		using Array = std::vector<T>;
		using Offset = std::vector<int>;
		using Pair = std::pair<const T*, const T*>;
		Array		_value;		//! 全要素配列
		Offset		_offset;	//! 各グループのオフセット(nGroup + 1)

		public:
			OffsetIndex(int nReserveValue=0, int nReserveOffset=0):
				_value(nReserveValue),
				_offset(nReserveOffset)
			{
				clear();
			}
			void clear() {
				_value.clear();
				_offset.resize(1);
				_offset[0] = 0;
			}
			void pushValue(const T& val) {
				_value.push_back(val);
			}
			//! 現在のグループを閉じて次のグループを開始
			void finishGroup() {
				auto sz = _value.size();
				if(_offset.back() < static_cast<int>(sz))
					_offset.push_back(sz);
			}
			int getNGroup() const { return _offset.size()-1; }
			int getGroupSize(int n) const { return _offset[n+1]-_offset[n]; }
			const Array& getArray() const { return _value; }
			const Offset& getOffset() const { return _offset; }
			Array& refArray() { return _value; }
			Offset& refOffset() { return _offset; }
			//! 特定のグループ範囲を取り出す
			Pair getGroup(int n) const {
				const int *pBegin = _value.data() + _offset[n],
						*pEnd = _value.data() + _offset[n+1];
				return std::make_pair(pBegin, pEnd);
			}
	};
}

