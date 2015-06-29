#pragma once

namespace spn {
	//! リングカーソル
	template <class T>
	struct CycleVal {
		T	value,
			limit;

		static struct tagAsInRange {} AsInRange;
		//! 入力値が範囲を超えていないことを前提とする
		CycleVal(const T& val, const T& lim, tagAsInRange): value(val), limit(lim) {}
		CycleVal(const T& val, const T& lim): limit(lim) {
			set(val);
		}
		const T& get() const { return value; }
		void set(const T& v) {
			value = v % limit;
			if(v < 0)
				value = limit + v;
		}
		CycleVal operator + (const T& v) const {
			return CycleVal(value+v, limit);
		}
		CycleVal operator - (const T& v) const {
			return CycleVal(value-v, limit);
		}
		operator const T& () const {
			return value;
		}
		CycleVal& operator += (const T& v) {
			set(value + v);
			return *this;
		}
		CycleVal& operator -= (const T& v) {
			set(value - v);
			return *this;
		}
		bool operator == (const T& v) const { return value == v; }
		bool operator != (const T& v) const { return value != v; }
		CycleVal& operator = (const T& v) {
			set(v);
			return *this;
		}
		//! 事前に範囲を指定しておき、実行時には値だけを指定してCycleValを返す
		static auto MakeValueF(const T& lim) {
			return [lim](const T& v){ return CycleVal(v, lim); };
		}
	};
	template <class T>
	typename CycleVal<T>::tagAsInRange CycleVal<T>::AsInRange;
	using CyInt = CycleVal<int>;
}
