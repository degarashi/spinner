#pragma once
#include <memory>

namespace spn {
	// スマートポインタの型判定
	template <class T>
	struct IsUniquePointer : std::false_type {};
	template <class... Ts>
	struct IsUniquePointer<std::unique_ptr<Ts...>> : std::true_type {};
	template <class T>
	struct IsSharedPointer : std::false_type {};
	template <class... Ts>
	struct IsSharedPointer<std::shared_ptr<Ts...>> : std::true_type {};
	template <class T>
	struct IsSmartPointer : std::integral_constant<bool, IsUniquePointer<T>::value | IsSharedPointer<T>::value> {};
}
