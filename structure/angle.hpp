#pragma once
#include "spn_math.hpp"
#include <type_traits>

namespace spn {
	// Degree角度を示すタグ構造体
	struct Degree_t {};
	// Radian角度を示すタグ構造体
	struct Radian_t {};

	//! 角度変換ヘルパークラス
	template <class From, class To>
	struct ConvertAngle;
	// 同じ単位なので変換の必要なし
	template <class TAG>
	struct ConvertAngle<TAG, TAG> {
		template <class T>
		T operator()(T ang) const {
			return ang;
		}
	};
	template <>
	struct ConvertAngle<Degree_t, Radian_t> {
		template <class T>
		T operator()(T deg) const {
			return deg / 180 * Pi<T>;
		}
	};
	template <>
	struct ConvertAngle<Radian_t, Degree_t> {
		template <class T>
		T operator()(T rad) const {
			return rad / Pi<T> * 180;
		}
	};

	template <class TAG>
	struct OneLoop;
	template <>
	struct OneLoop<Degree_t> {
		template <class T>
		constexpr static T value = T(360);
	};
	template <>
	struct OneLoop<Radian_t> {
		template <class T>
		constexpr static T value = T(2*Pi<T>);
	};

	//! 角度クラス
	/*! 混同を避けるために数値への変換は明示的
		他形式との変換は暗黙的 */
	template <class TAG, class V>
	class Angle {
		private:
			using tag_type = TAG;
			using value_type = V;
			value_type	_angle;
			constexpr static value_type Oneloop = OneLoop<tag_type>::template value<value_type>;
		public:
			Angle() = default;
			//! Degreeで角度指定
			template <class TAG2, class V2>
			Angle(const Angle<TAG2,V2>& ang) {
				_angle = ConvertAngle<TAG2, TAG>()(ang.get());
			}
			//! floatで直接角度指定
			explicit Angle(value_type ang) {
				_angle = ang;
			}
			Angle& operator = (const Angle& ang) = default;
			template <class TAG2, class V2>
			Angle& operator = (const Angle<TAG2,V2>& ang) {
				_angle = Angle(ang);
				return *this;
			}

			// 浮動少数点数型なら明示的に角度を出力
			template <class T2, class=std::enable_if_t<std::is_floating_point<T2>::value>>
			explicit operator T2() const {
				return _angle;
			}
			// 他のAngle形式へは暗黙的に変換できる
			template <class TAG2, class V2>
			operator Angle<TAG2,V2>() const {
				return Angle<TAG2,V2>(*this);
			}
			template <class TAG2, class V2=value_type>
			Angle<TAG2,V2> convert() const {
				return Angle<TAG2,V2>(*this);
			}
			void set(value_type ang) {
				_angle = ang;
			}
			value_type get() const {
				return _angle;
			}
			//! 角度をループ(0から2Piの間に収める)
			void single() {
				float sign = (_angle > 0) ? 1 : -1;
				_angle *= sign;
				_angle = std::fmod(_angle, Oneloop);
				_angle *= sign;
			}
			void rangeValue(V low, V high) {
				if(_angle < low)
					_angle = low;
				else if(_angle > high)
					_angle = high;
			}
			//! 角度に制限をつける
			void range(const Angle& low, const Angle& high) {
				if(_angle < low.get())
					_angle = low.get();
				else if(_angle > high.get())
					_angle = high.get();
			}

			template <class TAG2, class V2>
			Angle operator + (const Angle<TAG2,V2>& ang) const {
				return Angle(_angle + Angle(ang).get());
			}
			template <class TAG2, class V2>
			Angle& operator += (const Angle<TAG2,V2>& ang) {
				_angle += Angle(ang).get();
				return *this;
			}
			template <class TAG2, class V2>
			Angle operator - (const Angle<TAG2,V2>& ang) const {
				return Angle(_angle - Angle(ang).get());
			}
			template <class TAG2, class V2>
			Angle& operator -= (const Angle<TAG2,V2>& ang) {
				_angle -= Angle(ang).get();
				return *this;
			}
			Angle operator * (value_type r) const {
				return Angle(_angle * r);
			}
			Angle& operator *= (value_type r) {
				_angle *= r;
				return *this;
			}
			Angle operator / (value_type r) const {
				return Angle(_angle / r);
			}
			Angle& operator /= (value_type r) {
				_angle /= r;
				return *this;
			}
	};

	template <class T>
	using Degree = Angle<Degree_t, T>;
	using DegF = Degree<float>;
	using DegD = Degree<double>;
	template <class T>
	using Radian = Angle<Radian_t, T>;
	using RadF = Radian<float>;
	using RadD = Radian<double>;
}

