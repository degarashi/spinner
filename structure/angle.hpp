#pragma once
#include "spn_math.hpp"
#include <type_traits>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include "serialization/traits.hpp"
#include "../random/rotation.hpp"

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
			template <class... Ts>
			static Angle Random(Ts&&... ts) {
				return random::GenRAngle<Angle>(std::forward<Ts>(ts)...);
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

			// -------- Luaへのエクスポート用 --------
			using Deg_t = Angle<Degree_t, value_type>;
			using Rad_t = Angle<Radian_t, value_type>;
			Angle luaAddD(const Deg_t& a) const {
				return *this + a;
			}
			Angle luaAddR(const Rad_t& a) const {
				return *this + a;
			}
			Angle luaSubD(const Deg_t& a) const {
				return *this - a;
			}
			Angle luaSubR(const Rad_t& a) const {
				return *this - a;
			}
			Deg_t luaToDegree() const {
				return *this;
			}
			Rad_t luaToRadian() const {
				return *this;
			}
			Angle luaMulF(float s) const {
				return *this * s;
			}
			Angle luaDivF(float s) const {
				return *this / s;
			}
			Angle luaInvert() const {
				return -*this;
			}
			bool luaLessthan(const Angle& a) const {
				return *this < a;
			}
			bool luaLessequal(const Angle& a) const {
				return *this <= a;
			}
			bool luaEqual(const Angle& a) const {
				return *this == a;
			}
			std::string luaToString() const {
				return ToString(*this);
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

	template <class TAG, class V>
	std::ostream& operator << (std::ostream& os, const Angle<TAG,V>& ang) {
		return os << ang.get() << "(" << AngleInfo<TAG>::name_short << ")";
	}
}
BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class)(class), spn::Angle, object_serializable)
BOOST_CLASS_TRACKING_TEMPLATE((class)(class), spn::Angle, track_never)

// ------------- Angle系関数 -------------
namespace spn {
	template <int N, bool A>
	struct VecT;
	using Vec2 = VecT<2,false>;

	//! dirAを基準に反時計回りに増加する値を返す
	/*! \param[in] dir 値を算出したい単位ベクトル
		\param[in] dirA 基準の単位ベクトル
		\return 角度に応じた0〜4の値(一様ではない) */
	float AngleValueNL(const Vec2& dir, const Vec2& dirA);
	//! 上方向を基準としたdirの角度を返す(半時計周り)
	/*! \return 0〜2*PI未満の角度値 */
	RadF AngleValue(const Vec2& dir);
	//! ang0,ang1における-oneloop以上oneloop未満の差分角度値を計算
	float AngleLerpValueDiff(float ang0, float ang1, const float oneloop);
	template <class Proc>
	float AngleLerpValue(float ang0, float ang1, const Proc& proc, const float oneloop) {
		auto diff = AngleLerpValueDiff(ang0, ang1, oneloop);
		return ang0 + proc(diff);
	}
	//! 上方向を0度とし、反時計回り方向に指定角度回転したベクトルを返す
	Vec2 VectorFromAngle(const RadF& ang);
	//! 2つの角度値をang0 -> ang1の線形補間
	template <class T>
	T AngleLerp(const T& ang0, const T& ang1, float r) {
		auto fn = [r](auto&& v){ return v*r; };
		return T(AngleLerpValue(ang0.get(), ang1.get(), fn, T::OneRotationAng));
	}
	//! ang0からang1へ向けてmaxDiff以下の分だけ近づける
	template <class T>
	T AngleMove(const T& ang0, const T& ang1, const T& maxDiff) {
		auto fn = [mdiff = maxDiff.get()](auto&& v) {
			if(std::abs(v) > mdiff)
				return (v > 0) ? mdiff : -mdiff;
			return decltype(mdiff)(v);
		};
		return T(AngleLerpValue(ang0.get(), ang1.get(), fn, T::OneRotationAng));
	}
}
