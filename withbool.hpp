#pragma once

namespace spn {
	template <class T, bool B>
	struct __WithBool : T {
		using T::T;
		__WithBool(const __WithBool<T,B>& wb): T(static_cast<const T&>(wb)) {}
		__WithBool(__WithBool<T,B>&& wb): T(std::move(static_cast<const T&>(wb))) {}
	};
	// bool型とのセットは駄目
	template <bool B>
	struct __WithBool<bool, B>;
	template <class T>
	struct __WithBool<T, true> {
		T	_value;

		template <class T2>
		__WithBool(T2&& t): _value(std::forward<T2>(t)) {}
		__WithBool(const __WithBool<T,true>& wb): _value(wb._value) {}
		__WithBool(__WithBool<T,true>&& wb): _value(std::move(wb._value)) {}

		operator T& () { return _value; }
		operator const T& () const { return _value; }
	};
	template <class T>
	struct __WithBool<T&, false> {
		T& _ref;

		__WithBool(T& t): _ref(t) {}
		__WithBool(const __WithBool<T&,false>& wb): _ref(wb._ref) {}

		T* operator -> () { return &_ref; }
		const T* operator -> () const { return &_ref; }
		T& operator * () { return _ref; }
		const T& operator * () const { return _ref; }
	};
	template <class T>
	struct __WithBool<T*, false> {
		T* _ptr;

		__WithBool(T* t): _ptr(t) {}
		__WithBool(const __WithBool<T*,false>& wb): _ptr(wb._ptr) {}

		T* operator -> () { return _ptr; }
		const T* operator -> () const { return _ptr; }
		T& operator * () { return *_ptr; }
		const T& operator * () const { return *_ptr; }
	};
	template <class T>
	struct _WithBool : __WithBool<T, std::is_scalar<T>::value> {
		using base_type = __WithBool<T, std::is_scalar<T>::value>;
		using base_type::base_type;
	};

	//! boolと任意値のセット
	/*! operator bool()でbool値を取り出せる */
	template <class T>
	struct WithBool : _WithBool<T> {
		bool	_bool;

		WithBool(const WithBool& wb): _WithBool<T>(static_cast<const _WithBool<T>&>(wb)), _bool(wb._bool) {}
		WithBool(WithBool&& wb): _WithBool<T>(std::move(static_cast<_WithBool<T>&>(wb))), _bool(wb._bool) {}
		template <class T2>
		WithBool(bool b, T2&& t):_WithBool<T>(std::forward<T2>(t)), _bool(b) {}
		operator bool () const { return _bool; }
		bool boolean() const { return _bool; }
	};

	template <class T>
	inline WithBool<T> make_wb(bool b, const T& t) {
		return WithBool<T>(b, t);
	}
	template <class T>
	inline WithBool<T> make_wb(bool b, T&& t) {
		return WithBool<T>(b, std::forward<T>(t));
	}
}
