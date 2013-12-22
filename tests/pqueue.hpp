#pragma once
#include "../pqueue.hpp"
#include <type_traits>

namespace spn {
	namespace test {
		//! non-copyableな値
		template <class T>
		struct MoveOnly {
			using move_type = T;
			T	value;

			MoveOnly(const T& v): value(v) {}
			MoveOnly(T&& v): value(std::move(v)) {}
			MoveOnly(const MoveOnly&) = delete;
			MoveOnly(MoveOnly&& m): value(std::move(m.value)) {}
			void operator = (const MoveOnly&) = delete;
			MoveOnly& operator = (MoveOnly&& m) {
				value = std::move(m.value);
				return *this;
			}
			template <class T2>
			static const T2& _getValue(const T2& t, ...) {
				return t;
			}
			template <class T3>
			static auto _getValue(const MoveOnly<T3>& t, ...) -> decltype(t.getValue()){
				return t.getValue();
			}
			auto getValue() const -> decltype(_getValue(value, nullptr)) {
				return _getValue(value, nullptr);
			}
			bool operator == (const MoveOnly& m) const {
				return getValue() == m.getValue();
			}
			bool operator < (const MoveOnly& m) const {
				return getValue() < m.getValue();
			}
		};

		void PQueue();
		void Math();
	}
}
