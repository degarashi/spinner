#pragma once
#include <boost/operators.hpp>

namespace spn {
	//! 固定長配列の大小比較
	template <class T, int N>
	struct ValueArray :
		boost::equality_comparable<ValueArray<T,N>>,
		boost::less_than_comparable<ValueArray<T,N>>
	{
		using this_t = ValueArray<T, N>;
		constexpr static int length = N;

		template <class T2, int N2, class CMP>
		static bool CompareArray(const T2 (&ar0)[N2], const T2 (&ar1)[N2], CMP&& cmp) {
			for(int i=0 ; i<N2-1 ; i++) {
				auto &a0 = ar0[i],
					 &a1 = ar1[i];
				if(a0 != a1)
					return cmp(a0, a1);
			}
			return cmp(ar0[N2-1], ar1[N2-1]);
		}

		T	value[N];
		bool operator == (const ValueArray& v) const {
			return CompareArray(value, v.value, std::equal_to<T>());
		}
		bool operator < (const ValueArray& v) const {
			return CompareArray(value, v.value, std::less<T>());
		}

		ValueArray() = default;
		ValueArray(const ValueArray& v) {
			for(int i=0 ; i<N ; i++)
				value[i] = v.value[i];
		}
		ValueArray& operator = (const ValueArray& v) {
			for(int i=0 ; i<N ; i++)
				value[i] = v.value[i];
			return *this;
		}
	};
}

