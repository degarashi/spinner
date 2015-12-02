#pragma once
#include <algorithm>

namespace spn {
	template <class T>
	struct Wrapper {
		T	_value;

		template <class... Args>
		Wrapper(Args&&... args):
			_value(std::forward<Args>(args)...)
		{}
		operator const T& () const {
			return _value;
		}
		operator T& () {
			return _value;
		}
		operator T&& () {
			return std::move(_value);
		}
		Wrapper& operator = (const T& t) {
			_value = t;
			return *this;
		}
		Wrapper& operator = (T&& t) {
			_value = std::move(t);
			return *this;
		}
	};
}
