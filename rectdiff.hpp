#pragma once
#include <functional>
#include "size.hpp"
#include "ulps.hpp"

namespace spn {
	namespace rect {
		//! 指定範囲内で値をループし、区画のローカル値を返す
		template <
			class T,
			 std::enable_if_t< std::is_integral<T>::value >*& = Enabler
		>
		T LoopValue(const T& t, const T& w) {
			if(t < 0)
				return (w-1)+((t+1)%w);
			return t % w;
		}
		template <
			class T,
			 std::enable_if_t< std::is_floating_point<T>::value >*& = Enabler
		>
		T LoopValue(const T& t, const T& w) {
			const auto m = std::fmod(t, w);
			if(t < 0) {
				if(std::abs(m) < std::abs(ULPs_Increment(w)-w))
					return 0;
				return w+m;
			}
			return m;
		}
		//! 指定範囲内で値をループし、区画インデックスを返す
		template <
			class T,
			std::enable_if_t< std::is_integral<T>::value >*& = Enabler
		>
		T LoopValueD(const T& t, const T& w) {
			if(t < 0)
				return ((t+1)/w)-1;
			return t/w;
		}
		template <
			class T,
			std::enable_if_t< std::is_floating_point<T>::value >*& = Enabler
		>
		T LoopValueD(const T& t, const T& w) {
			return std::floor(t / w);
		}
		using Rect_cb = std::function<void (const Rect&)>;
		//! 矩形の差分のL字を最大2つの矩形として算出
		void IncrementalRect(const Rect& base, int dx, int dy, const Rect_cb& cb);
		using DRect_cb = std::function<void (const Rect&, const Rect&)>;
		//! 矩形を別の矩形へマッピングする際に境界をまたぐ部分を分割
		void DivideRect(const Size& size, const Rect& request, const DRect_cb& cb);
	}
}
