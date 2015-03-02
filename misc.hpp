#pragma once
#include "vector.hpp"
#include "matrix.hpp"
#include "quat.hpp"
#include "bits.hpp"
#include "abstbuff.hpp"
#include "error.hpp"
#include "optional.hpp"
#include <cassert>
#include <cstring>
#include <boost/regex_fwd.hpp>
#include "structure/angle.hpp"

namespace spn {
	//! Y軸を上とした時のZベクトルに対するXベクトルを算出, またはベクトルに垂直なベクトルを得る
	template <bool A>
	VecT<3,A> GetVerticalVec(const VecT<3,A>& zvec) {
		using VT = VecT<3,A>;
		auto vt = zvec % VT(0,1,0);
		if(vt.len_sq() < 1e-6f)
			vt = zvec % VT(1,0,0);
		return vt.normalization();
	}
	template <bool A>
	spn::Optional<VecT<3,A>> NormalFromPoints(const VecT<3,A>& v0,const VecT<3,A>& v1, const VecT<3,A>& v2) {
		auto tmp = (v1 - v0) % (v2 - v0);
		if(tmp.len_sq() < 1e-6f)
			return spn::none;
		return tmp.normalization();
	}
	//! 任意の戻り値を返す関数 (定義のみ)
	template <class T>
	T ReturnT();
	//! 関数の戻り値型を取得
	/*! ReturnType<T>::type */
	template <class T>
	struct ReturnType;
	template <class RT, class... Args>
	struct ReturnType<RT (*)(Args...)> {
		using type = RT; };
	template <class RT, class OBJ, class... Args>
	struct ReturnType<RT (OBJ::*)(Args...)> {
		using type = RT; };
	template <class RT, class OBJ, class... Args>
	struct ReturnType<RT (OBJ::*)(Args...) const> {
		using type = RT; };
	inline uint32_t ARGBtoRGBA(uint32_t val) {
		return (val & 0xff00ff00) | ((val>>16)&0xff) | ((val&0xff)<<16);
	}
	inline uint32_t SetAlphaR(uint32_t val) {
		return (val & 0x00ffffff) | ((val&0xff)<<24);
	}

	//! ポインタ読み替え変換
	template <class T0, class T1>
	inline T1 RePret(const T0& val) {
		static_assert(sizeof(T0) == sizeof(T1), "invalid reinterpret value");
		return *reinterpret_cast<const T1*>(&val);
	}
	//! aがb以上だったらaからsizeを引いた値を返す
	inline int CndSub(int a, int b, int size) {
		return a - (size & ((b - a - 1) >> 31));
	}
	//! aがb以上だったらaからbを引いた値を返す
	inline int CndSub(int a, int b) {
		return CndSub(a, b, b);
	}
	//! aがbより小さかったらaにsizeを足した値を返す
	inline int CndAdd(int a, int b, int size) {
		return a + (size & ((a - b) >> 31));
	}
	//! aが0より小さかったらaにsizeを足した値を返す
	inline int CndAdd(int a, int size) {
		return CndAdd(a, 0, size);
	}
	//! aがlowerより小さかったらsizeを足し、upper以上だったらsizeを引く
	inline int CndRange(int a, int lower, int upper, int size) {
		return a + (size & ((a - lower) >> 31))
					- (size & ((upper - a - 1) >> 31));
	}
	//! aが0より小さかったらsizeを足し、size以上だったらsizeを引く
	inline int CndRange(int a, int size) {
		return CndRange(a, 0, size, size);
	}
	//! ポインタの読み替え
	template <class T, class T2>
	T* ReinterpretPtr(T2* ptr) {
		using TD = typename std::decay<T2>::type;
		static_assert(std::is_integral<TD>::value || std::is_floating_point<TD>::value, "typename T must number");
		return reinterpret_cast<T*>(ptr);
	}
	//! リファレンスの読み替え
	template <class T, class T2>
	T& ReinterpretRef(T2& val) {
		return *reinterpret_cast<T*>(&val);
	}
	//! 値の読み替え
	template <class T, class T2>
	T ReinterpretValue(const T2& val) {
		return *reinterpret_cast<const T*>(&val);
	}
	constexpr uint32_t FloatOne = 0x3f800000;		//!< 1.0fの、整数表現
	//! 引数がプラスなら1, マイナスなら-1を返す
	float inline PlusMinus1(float val) {
		auto ival = spn::ReinterpretValue<uint32_t>(val);
		return spn::ReinterpretValue<float>(FloatOne | (ival & 0x80000000));
	}
	//! PlusMinus1(float)の引数がintバージョン
	float inline PlusMinus1(int val) {
		return spn::ReinterpretValue<float>(FloatOne | ((val>>31) & 0x80000000)); }
	//! PlusMinus1(float)の引数がunsigned intバージョン
	float inline PlusMinus1(unsigned int val) { return PlusMinus1(static_cast<int>(val)); }

	//! dirAを基準に反時計回りに増加する値を返す
	/*! \param[in] dir 値を算出したい単位ベクトル
		\param[in] dirA 基準の単位ベクトル
		\return 角度に応じた0〜4の値(一様ではない) */
	inline float AngleValueNL(const Vec2& dir, const Vec2& dirA) {
		float d0 = dir.dot(dirA);
		if(dirA.cw(dir) <= -1e-6f)
			return d0+1 + 2;
		return 2.f-(d0+1);
	}
	//! 上方向を基準としたdirの角度を返す(半時計周り)
	inline RadF AngleValue(const Vec2& dir) {
		float ac0 = std::acos(Saturate(dir.y, 1.f));
		if(dir.x >= 1e-6f)
			return RadF(2*spn::PI - ac0);
		return RadF(ac0);
	}
	//! 2次元ベクトルを係数で混ぜ合わせる
	Vec2 MixVector(const float (&cf)[3], const Vec2& p0, const Vec2& p1, const Vec2& p2);
	//! 重心座標を計算
	void BarycentricCoord(float ret[3], const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& pos);
	//! クラメルの公式で使う行列式を計算
	float CramerDet(const Vec3& v0, const Vec3& v1, const Vec3& v2);
	float CramerDet(const Vec2& v0, const Vec2& v1);
	//! 3元1次方程式の解をクラメルの公式を用いて計算
	/*! @param detInv[in] 行列式(v0,v1,v2)の逆数 */
	Vec3 CramersRule(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& a0, float detInv);
	//! 2元1次方程式を計算
	Vec2 CramersRule(const Vec2& v0, const Vec2& v1, const Vec2& a0, float detInv);
	//! グラム・シュミットの正規直交化法
	void GramSchmidtOrth(Vec3& v0, Vec3& v1, Vec3& v2);

	struct YawPitchDist {
		RadF	yaw, pitch;
		float	distance;
		//! 方向ベクトルをYaw,Pitch,Distanceに分解
		static YawPitchDist FromPos(const spn::Vec3& pos);
		//! YawPitchDistの位置から座標原点を見る姿勢
		std::pair<spn::Vec3, spn::Quat> toOffsetRot() const;
	};
	std::ostream& operator << (std::ostream& os, const YawPitchDist& ypd);

	struct AffineParts {
		AVec3 offset,
				scale;
		AQuat rotation;
		//! アフィン成分分解
		static AffineParts Decomp(const AMat43& m);
	};
	//! ソースコード文字列に行番号を付ける
	/*! \param[in] src 元のソースコード
		\param[in] numOffset 最初の数値(0や負数だとその分は番号を振らない)
		\param[in] viewNum 行番号の表示を開始する位置
		\param[in] bPrevLR 最初に改行を入れるか
		\param[in] bPostLR 最後に改行を入れるか*/
	std::string AddLineNumber(const std::string& src, int numOffset, int viewNum, bool bPrevLR, bool bPostLR);

	using OPLinePos = spn::Optional<std::pair<int, typename std::string::const_iterator>>;
	//! regexに合致する最初の行
	/*!	\param[in] str 検索する文字列
		\param[in] re 検索パターン(行単位)
		\retval 合致する行が見つかった時: std::pair<先頭からの行番号, その位置>
		\retval 見つからなかった時: spn::none*/
	OPLinePos RegexFindFirst(const std::string& str, const boost::regex& re);
	//! regexに合致する最後の行
	OPLinePos RegexFindLast(const std::string& str, const boost::regex& re);
}
