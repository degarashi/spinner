#pragma once
#include <type_traits>

//! 特定のメソッドを持っているかチェック
/*! DEF_HASMETHOD(method)
	HasMethod_method<class_name>(nullptr) -> std::true_type or std::false_type */
#define DEF_HASMETHOD(method) \
	template <class T> \
	std::true_type HasMethod_##method(decltype(&T::method) = &T::method) { return std::true_type(); } \
	template <class T> \
	std::false_type HasMethod_##method(...) { return std::false_type(); } \
	template <class T> \
	using HasMethod_##method##_t = decltype(HasMethod_##method<T>(nullptr));

//! 特定のオーバーロードされたメソッドを持っているかチェック
/*! DEF_HASMETHOD_OV(name, method)
	予めクラスにspn::none method(...); を持たせておく
	暗黙の変換も含まれる
	HasMethod_name<class_name>() -> std::true_type or std::false_type */
#define DEF_HASMETHOD_OV(name, method, ...) \
	DEF_CHECKMETHOD_OV(name, method) \
	template <class T> \
	decltype(CheckMethod_##name<T, __VA_ARGS__>()) HasMethod_##name() { return decltype(CheckMethod_##name<T, __VA_ARGS__>())(); }
#define DEF_CHECKMETHOD_OV(name, method) \
	template <class T, class... Args> \
	std::integral_constant<bool, !std::is_same<::spn::none_t, decltype(std::declval<T>().method(::spn::ReturnT<Args>()...))>::value> CheckMethod_##name() { \
		return std::integral_constant<bool, !std::is_same<::spn::none_t, decltype(std::declval<T>().method(::spn::ReturnT<Args>()...))>::value>(); }

//! 特定の型又は定数値を持っているかチェック
/*! DEF_HASTYPE(type_name)
	HasType_type_name<class_name>(nullptr) -> std::true_type or std::false_type */
#define DEF_HASTYPE(name) \
	template <class T> \
	std::true_type HasType_##name(decltype(T::name)*) { return std::true_type(); } \
	template <class T> \
	std::false_type HasType_##name(...) { return std::false_type(); } \
	template <class T> \
	using HasType_##name##_t = decltype(HasType_##name<T>(nullptr));

