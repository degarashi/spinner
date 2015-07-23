#pragma once
#include <algorithm>
#include <tuple>
#include "../type.hpp"

namespace spn {
	//! std::tuple型に暗黙の変換できるか
	template <class T, class=typename T::Tuple>
	std::true_type IsConvertibleToTuple(T*);
	std::false_type IsConvertibleToTuple(...);

	//! std::tupleに変換可能か
	template <class T>
	struct IsTupleBased : decltype(IsConvertibleToTuple((T*)nullptr)) {};
	//! ベースのstd::tuple型を取り出す
	template <class... Ts>
	auto GetTuple(std::tuple<Ts...>*) -> std::tuple<Ts...>;
	template <class T>
	auto GetTuple(T* ptr, typename T::Tuple* tag=nullptr) -> decltype(GetTuple(tag));

	namespace detail {
		template <class OP, class A0, class... Args>
		decltype(auto) TupleForEach(OP&& /*op*/, std::integral_constant<int,0>, A0&& a0, Args&&... /*args*/);
		template <class OP, int N, class... Args>
		decltype(auto) TupleForEach(OP&& op, std::integral_constant<int,N>, Args&&... args);

		template <class OP, class... Ts,
					// 何れもstd::tuple型ではない
					class = std::enable_if_t<
						And_t<
							(!IsTupleBased<
								std::decay_t<Ts>
							>::value)...
						>::value
					>
				>
		void CallOP(OP&& op, Ts&&... ts) {
			op(std::forward<Ts>(ts)...);
		}
		template <class OP, class... Ts,
					// 全てstd::tuple型
					class = std::enable_if_t<
								And_t<
									IsTupleBased<
										std::decay_t<Ts>
									>::value...
								>::value>
					>
		decltype(auto) CallOP(OP&& op, Ts&&... ts) {
			using T = typename FirstType<std::decay_t<Ts>...>::type;
			using Tuple = decltype(GetTuple((T*)nullptr));
			constexpr int sz = std::tuple_size<Tuple>::value;
			return ::spn::detail::TupleForEach(op, std::integral_constant<int,sz>(), std::forward<Ts>(ts)...);
		}

		template <class OP, class A0, class... Args>
		decltype(auto) TupleForEach(OP&& /*op*/, std::integral_constant<int,0>, A0&& a0, Args&&... /*args*/) {
			return std::forward<A0>(a0);
		}
		template <class OP, int N, class... Args>
		decltype(auto) TupleForEach(OP&& op, std::integral_constant<int,N>, Args&&... args) {
			CallOP(op, std::get<N-1>((decltype(GetTuple((std::decay_t<Args>*)nullptr))&)args)...);
			return TupleForEach(op, std::integral_constant<int,N-1>(), std::forward<Args>(args)...);
		}
	}
	template <class OP, class... Ts>
	decltype(auto) TupleForEach(OP&& op, Ts&&... ts) {
		using T = typename FirstType<std::decay_t<Ts>...>::type;
		using Tuple = decltype(GetTuple((T*)nullptr));
		constexpr int sz = std::tuple_size<Tuple>::value;
		return detail::TupleForEach(op, std::integral_constant<int,sz>(), std::forward<Ts>(ts)...);
	}

	static void* Enabler2;
	template <class T, typename std::enable_if_t< !IsTupleBased< std::decay_t<T> >::value>*& = Enabler2 >
	void TupleZeroFill(T& t) {
		t = 0;
	}
	template <class T, typename std::enable_if_t< IsTupleBased< std::decay_t<T> >::value>*& = Enabler2 >
	void TupleZeroFill(T& p) {
		TupleForEach([](auto& a){
			TupleZeroFill(a);
		}, p);
	}
}

#define DefOp(op) \
	template <class T0, class T1, \
			class=std::enable_if_t< spn::IsTupleBased< std::decay_t<T0> >::value>, \
			class=std::enable_if_t< spn::IsTupleBased< std::decay_t<T1> >::value>> \
	decltype(auto) operator op##= (T0& p0, const T1& p1) { \
		const auto fn = [](auto& a, const auto& b){ a op##= b; }; \
		return spn::TupleForEach(fn, p0, p1); \
	} \
	template <class T0, class T1, \
			class=std::enable_if_t< spn::IsTupleBased< std::decay_t<T0> >::value>, \
			class=std::enable_if_t< spn::IsTupleBased< std::decay_t<T1> >::value>> \
	auto operator op (const T0& p0, const T1& p1) { \
		const auto fn = [](const auto& a, const auto& b){ return a op b; }; \
		auto tmp = p0; \
		spn::TupleForEach(fn, tmp, p1); \
		return tmp; \
	}
DefOp(+)
DefOp(-)
DefOp(*)
DefOp(/)
DefOp(%)
#undef DefOp
