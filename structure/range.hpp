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
		#define DEF_OP(op) \
			Range& operator op##= (const T& t) { \
				from op##= t; \
				to op##= t; \
				return *this; \
			} \
			Range operator op (const T& t) const { \
				return Range(from op t, to op t); \
			}
		DEF_OP(+)
		DEF_OP(-)
		DEF_OP(*)
		DEF_OP(/)
		#undef DEF_OP
	};
	using RangeI = Range<int>;
	using RangeF = Range<float>;
}

