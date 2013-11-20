#pragma once
#include <type_traits>

namespace spn {
	struct none_t{};
	//! rvalue-reference wrapper
	template <class T>
	struct _RRef {
		T& value;
		_RRef(T& v): value(v) {}
		_RRef(const _RRef& r): value(r.value) {}
		int get() { return 0; }
		operator T& () { return value; }
	};
	template <class T>
	struct _RRef<T&&> {
		T&& value;
		_RRef(const _RRef& r): value(std::move(r.value)) {}
		_RRef(T&& v): value(std::forward<T>(v)) {}
		operator T&& () { return std::move(value); }
		int get() { return 1; }
	};
	template <class T>
	auto RRef(T&& t) -> _RRef<decltype(std::forward<T>(t))> {
		return _RRef<T&&>(std::forward<T>(t));
	}

	template <class... Ts>
	struct ArgHolder {
		template <class CB, class... TsA>
		auto reverse(CB cb, TsA&&... tsa) -> decltype(cb(std::forward<TsA>(tsa)...)) {
			return cb(std::forward<TsA>(tsa)...);
		}
		template <class CB, class... TsA>
		auto inorder(CB cb, TsA&&... tsa) -> decltype(cb(std::forward<TsA>(tsa)...)) {
			return cb(std::forward<TsA>(tsa)...);
		}
	};
	template <class T0, class... Ts>
	struct ArgHolder<T0,Ts...> {
		template <class T>
		struct Inner {
			/*! T&& -> T&&
			 *	T& -> T&
			 *	T* -> T*
			 *	T = const T& */
			using type = typename std::conditional<!std::is_reference<T>::value  && !std::is_pointer<T>::value, const T&, T>::type;
		};
		using Type = typename Inner<T0>::type;
		Type				_value;
		using Lower = ArgHolder<Ts...>;
		Lower				_other;

		template <class T0A, class... TsA>
		ArgHolder(T0A&& t0, TsA&&... tsa): _value(std::forward<T0A>(t0)), _other(std::forward<TsA>(tsa)...) {}
		template <class CB, class... TsA>
		auto reverse(CB cb, TsA&&... tsa) -> decltype(_other.reverse(cb, std::forward<Type>(_value), std::forward<TsA>(tsa)...)) {
			return _other.reverse(cb, std::forward<Type>(_value), std::forward<TsA>(tsa)...);
		}
		template <class CB, class... TsA>
		auto inorder(CB cb, TsA&&... tsa) -> decltype(_other.inorder(cb, std::forward<TsA>(tsa)..., std::forward<Type>(_value))) {
			return _other.inorder(cb, std::forward<TsA>(tsa)..., std::forward<Type>(_value));
		}
	};
	//! 引数の順序を逆にしてコール
	template <class CB, class... Ts>
	auto ReversedArg(CB cb, Ts&&... ts) -> decltype(ArgHolder<decltype(std::forward<Ts>(ts))...>(std::forward<Ts>(ts)...).reverse(cb)) {
		return ArgHolder<decltype(std::forward<Ts>(ts))...>(std::forward<Ts>(ts)...).reverse(cb);
	}
}
