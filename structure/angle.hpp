#pragma once
#include "spn_math.hpp"
#include <type_traits>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include "serialization/traits.hpp"

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
	struct AngleInfo;
	template <>
	struct AngleInfo<Degree_t> {
		template <class T>
		constexpr static T onerotation = T(360);
		const static char *name,
							*name_short;
	};
	template <>
	struct AngleInfo<Radian_t> {
		template <class T>
		constexpr static T onerotation = T(2*Pi<T>);
		const static char *name,
							*name_short;
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

			friend class boost::serialization::access;
			template <class Ar>
			void serialize(Ar& ar, const unsigned int /*ver*/) {
				ar & BOOST_SERIALIZATION_NVP(_angle);
			}
		public:
			constexpr static value_type OneRotationAng = AngleInfo<tag_type>::template onerotation<value_type>;
			Angle() = default;
			//! Degreeで角度指定
			template <class TAG2, class V2>
			Angle(const Angle<TAG2,V2>& ang) {
				_angle = ConvertAngle<TAG2, TAG>()(ang.get());
			}
			//! floatで直接角度指定
			explicit constexpr Angle(value_type ang):
				_angle(ang)
			{}
			constexpr static Angle Rotation(value_type r=value_type(1)) {
				return Angle(OneRotationAng * r);
			}
			Angle& operator = (const Angle& ang) = default;
			template <class TAG2, class V2>
			Angle& operator = (const Angle<TAG2,V2>& ang) {
				_angle = Angle(ang).get();
				return *this;
			}

			// 浮動少数点数型なら明示的に角度を出力
			template <class T2, class=std::enable_if_t<std::is_floating_point<T2>::value>>
			explicit operator T2() const {
				return _angle;
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
				auto ang = std::fmod(_angle, OneRotationAng);
				if(ang < 0)
					ang += OneRotationAng;
				_angle = ang;
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
			Angle operator -() const {
				return Angle(-_angle);
			}

			// ---- 比較演算子の定義 ----
			#define DEF_ANGLE_OPERATOR(op) \
				template <class TAG2, class V2> \
				bool operator op (const Angle<TAG2,V2>& ang) const { \
					return _angle op Angle(ang).get(); \
				}
			DEF_ANGLE_OPERATOR(==)
			DEF_ANGLE_OPERATOR(!=)
			DEF_ANGLE_OPERATOR(<)
			DEF_ANGLE_OPERATOR(<=)
			DEF_ANGLE_OPERATOR(>)
			DEF_ANGLE_OPERATOR(>=)
			#undef DEF_ANGLE_OPERATOR
	};

	template <class T>
	using Degree = Angle<Degree_t, T>;
	using DegF = Degree<float>;
	using DegD = Degree<double>;
	template <class T>
	using Radian = Angle<Radian_t, T>;
	using RadF = Radian<float>;
	using RadD = Radian<double>;

	template <class TAG, class V>
	std::ostream& operator << (std::ostream& os, const Angle<TAG,V>& ang) {
		return os << ang.get() << "(" << AngleInfo<TAG>::name_short << ")";
	}
}
BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class)(class), spn::Angle, object_serializable)
BOOST_CLASS_TRACKING_TEMPLATE((class)(class), spn::Angle, track_never)

