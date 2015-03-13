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
	};
	using RangeI = Range<int>;
	using RangeF = Range<float>;
}

