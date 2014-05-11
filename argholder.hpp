#pragma once
#include <type_traits>
#include <cassert>

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
		ArgHolder() = default;
		ArgHolder(ArgHolder&& /*a*/) {}
		template <class CB, class... TsA>
		auto reverse(CB cb, TsA&&... tsa) -> decltype(cb(std::forward<TsA>(tsa)...)) {
			return cb(std::forward<TsA>(tsa)...);
		}
		template <class CB, class... TsA>
		auto inorder(CB cb, TsA&&... tsa) -> decltype(cb(std::forward<TsA>(tsa)...)) {
			return cb(std::forward<TsA>(tsa)...);
		}
	};
	//! 引数を一旦とっておいて後で関数に渡す
	/*! not reference, not pointerな型はrvalue_referenceで渡されるので2回以上呼べない */
	template <class T0, class... Ts>
	struct ArgHolder<T0,Ts...> {
		T0		_value;
		using Lower = ArgHolder<Ts...>;
		Lower	_other;

		// 2度呼び出しチェック
		#ifdef DEBUG
			bool	_bCall = false;
			void _check() {
				if(_bCall)
					assert(!"Invalid ArgHolder call (called twice)");
				_bCall = true;
			}
		#else
			void _check() {}
		#endif

		template <class T2>
		ArgHolder(ArgHolder<T2,Ts...>&& a): _value(std::move(a._value)), _other(std::move(a._other))
		#ifdef DEBUG
			, _bCall(a._bCall)
		#endif
		{}

		template <class T0A, class... TsA>
		ArgHolder(T0A&& t0, TsA&&... tsa): _value(std::forward<T0A>(t0)), _other(std::forward<TsA>(tsa)...) {}

		// T0がnot reference, not pointerな時はrvalue referenceで渡す
		template <class CB, class... TsA,
			class TT0 = T0, class = typename std::enable_if<std::is_object<TT0>::value>::type>
		auto reverse(CB cb, TsA&&... tsa) -> decltype(_other.reverse(cb, std::move(_value), std::forward<TsA>(tsa)...)) {
			_check();
			return _other.reverse(cb, std::move(_value), std::forward<TsA>(tsa)...);
		}
		// それ以外はそのまま渡す
		template <class CB, class... TsA,
			class TT0 = T0, class = typename std::enable_if<!std::is_object<TT0>::value>::type>
		auto reverse(CB cb, TsA&&... tsa) -> decltype(_other.reverse(cb, _value, std::forward<TsA>(tsa)...)) {
			return _other.reverse(cb, _value, std::forward<TsA>(tsa)...);
		}

		// T0がnot reference, not pointerな時はrvalue referenceで渡す
		template <class CB, class... TsA,
			class TT0 = T0, class = typename std::enable_if<std::is_object<TT0>::value>::type>
		auto inorder(CB cb, TsA&&... tsa) -> decltype(_other.inorder(cb, std::forward<TsA>(tsa)..., std::move(_value))) {
			_check();
			return _other.inorder(cb, std::forward<TsA>(tsa)..., std::move(_value));
		}
		// それ以外はそのまま渡す
		template <class CB, class... TsA,
			class TT0 = T0, class = typename std::enable_if<!std::is_object<TT0>::value>::type>
		auto inorder(CB cb, TsA&&... tsa) -> decltype(_other.inorder(cb, std::forward<TsA>(tsa)..., _value)) {
			return _other.inorder(cb, std::forward<TsA>(tsa)..., _value);
		}
	};
	//! 引数の順序を逆にしてコール
	template <class CB, class... Ts>
	auto ReversedArg(CB cb, Ts&&... ts) -> decltype(ArgHolder<decltype(std::forward<Ts>(ts))...>(std::forward<Ts>(ts)...).reverse(cb)) {
		return ArgHolder<decltype(std::forward<Ts>(ts))...>(std::forward<Ts>(ts)...).reverse(cb);
	}
}
