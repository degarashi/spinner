#pragma once
#include <functional>
#include "size.hpp"

namespace spn {
	namespace rect {
		//! 指定範囲内で値をループし、区画のローカル値を返す
		int LoopValue(const int t, const int w);
		//! 指定範囲内で値をループし、区画インデックスを返す
		int LoopValueD(const int t, const int w);
		using Rect_cb = std::function<void (const Rect&)>;
		//! 矩形の差分のL字を最大2つの矩形として算出
		void IncrementalRect(const Rect& base, int dx, int dy, const Rect_cb& cb);
		using DRect_cb = std::function<void (const Rect&, const Rect&)>;
		//! 矩形を別の矩形へマッピングする際に境界をまたぐ部分を分割
		void DivideRect(const Size& size, const Rect& request, const DRect_cb& cb);
	}
}
