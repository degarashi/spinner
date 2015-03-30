#pragma once
#include "error.hpp"

namespace spn {
	template <class T>
	struct Range {
		T	from,
			to;
		Range() = default;
		constexpr Range(const T& f, const T& t): from(f), to(t) {
			AssertP(Trap, from <= to, "invalid range");
		}
		//! 値の範囲チェック
		bool hit(const T& v) const {
			return v >= from
				&& v <= to;
		}
		//! 範囲が重なっているか
		bool hit(const Range& r) const {
			return !(to < r.from
					&& from > r.to);
		}
		bool operator == (const Range& r) const {
			return from == r.from
				&& to == r.to;
		}
		bool operator != (const Range& r) const {
			return !(this->operator == (r));
		}
	};
	using RangeI = Range<int>;
	using RangeF = Range<float>;
}

