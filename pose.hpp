#pragma once
#include "vector.hpp"
#include "quat.hpp"
#include "alignedalloc.hpp"
#include <boost/serialization/access.hpp>

namespace spn {
	enum POSERF {
		PRF_OFFSET = 0x01,
		PRF_ROTATE = 0x02,
		PRF_SCALE = 0x04,
		PRF_ALL = ~0,
		PRF_MAX = PRF_SCALE
	};
	//! 2次元姿勢クラス
	/*! 変換適用順序は[拡縮][回転][平行移動] */
	class Pose2D : public CheckAlign<16,Pose2D> {
		private:
			GAP_MATRIX_DEF(mutable, _finalMat, 3,2,
				(((Vec2, _ofs)))
				(((Vec2, _scale)))
				(((RadF, _angle), (mutable, uint32_t, _accum)))
			)

			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int /*ver*/) {
				ar & BOOST_SERIALIZATION_NVP(_ofs) & BOOST_SERIALIZATION_NVP(_angle) & BOOST_SERIALIZATION_NVP(_scale);
				if(Archive::is_loading::value) {
					_accum = std::rand();
					_rflag = PRF_ALL;
				}
			}
		protected:
			mutable uint32_t	_rflag;

			void _refresh() const;
			const float* _getPtr() const;
			void _setAsChanged();
		public:
			struct Value;
			struct TValue {
				Vec2	ofs, scale;
				RadF	ang;

				TValue() = default;
				TValue(const Value& v);
				TValue(const Pose2D& p);
				TValue& operator = (const Pose2D& p);
				TValue& operator = (const TValue& tv) = default;
				TValue& operator = (const Value& v);
			};
			struct Value {
				Pose2D	&_pose;
				Vec2	&ofs, &scale;
				RadF	&ang;

				Value(Pose2D& p);
				~Value();
				Value(const Value& v);
				Value& operator = (const TValue& tv);
				void operator = (const Value& v) = delete;
			};

			Pose2D();
			Pose2D(const Pose2D& p);
			Pose2D(const Vec2& pos, RadF ang, const Vec2& sc);
			Pose2D(const TValue& tv);
			void identity();

			// ---- Lua互換用メソッド ----
			AMat33 luaGetToWorld() const;
			AMat33 luaGetToLocal() const;
			bool equal(const Pose2D& ps) const;
			std::string toString() const;
			// ---- getter method ----
			const Vec2& getOffset() const;
			const Vec2& getScale() const;
			const AMat32& getToWorld() const;
			AMat32 getToLocal() const;
			uint32_t getAccum() const;
			RadF getAngle() const;
			// ---- setter method ----
			void setAll(const Vec2& ofs, RadF ang, const Vec2& sc);
			void setScale(const Vec2& ofs);
			void setAngle(RadF ang);
			void setOffset(const Vec2& ofs);
			// ---- reference method ----
			Vec2& refOffset();
			Vec2& refScale();
			RadF& refAngle();
			// ---- helper function ----
			void moveUp(float speed);
			void moveDown(float speed);
			void moveLeft(float speed);
			void moveRight(float speed);
			Vec2 getUp() const;
			Vec2 getRight() const;
			void setUp(const Vec2& up);

			Pose2D lerp(const Pose2D& p1, float t) const;
			Value refValue();
			void apply(const TValue& t);

			Pose2D& operator = (const Pose2D& ps);
			Pose2D& operator = (const TValue& tv);
			bool operator == (const Pose2D& ps) const;
			bool operator != (const Pose2D& ps) const;
			friend std::ostream& operator << (std::ostream&, const Pose2D&);
	};
	std::ostream& operator << (std::ostream& os, const Pose2D& ps);

	//! 3次元姿勢クラス
	class Pose3D : public CheckAlign<16,Pose3D> {
		private:
			AVec3			_ofs;
			AQuat			_rot;
			AVec3			_scale;

			GAP_MATRIX_DEF(mutable, _finalMat, 4,3,
				(((uint32_t, _accum)))
				(((mutable, uint32_t, _rflag)))
			)

			void _refresh() const;

			constexpr static float TURN_THRESHOLD = 1e-5f;
			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int /*ver*/) {
				ar & BOOST_SERIALIZATION_NVP(_ofs) & BOOST_SERIALIZATION_NVP(_rot) & BOOST_SERIALIZATION_NVP(_scale);
				if(Archive::is_loading::value) {
					_accum = std::rand();
					_rflag = PRF_ALL;
				}
			}

		protected:
			const float* _getPtr() const;
			void _setAsChanged();
		public:
			struct Value;
			struct TValue {
				Vec3	ofs, scale;
				Quat	rot;

				TValue() = default;
				TValue(const Value& v);
				TValue(const Pose3D& p);
				TValue& operator = (const TValue& tv) = default;
				TValue& operator = (const Value& v);
				TValue& operator = (const Pose3D& p);
			};
			struct Value {
				Pose3D	&_pose;
				AVec3	&ofs, &scale;
				AQuat	&rot;

				Value(Pose3D& p);
				~Value();
				Value(const Value& v);
				Value& operator = (const TValue& tv);
				void operator = (const Value& v) = delete;
			};

			Pose3D();
			Pose3D(const Pose3D& p);
			Pose3D(const AVec3& pos, const AQuat& rot, const AVec3& sc);
			Pose3D(const TValue& tv);
			Pose3D(const AMat43& m);
			void identity();

			// ---- Lua互換用メソッド ----
			AMat44 luaGetToWorld() const;
			AMat44 luaGetToLocal() const;
			bool equal(const Pose3D& p) const;
			std::string toString() const;
			// ---- helper function ----
			//! 前方への移動(XZ平面限定)
			void moveFwd2D(float speed);
			//! サイド移動(XZ平面限定)
			void moveSide2D(float speed);
			//! 前方への移動(軸フリー)
			void moveFwd3D(float speed);
			//! サイド移動(軸フリー)
			void moveSide3D(float speed);

			//! 方向転換(軸指定)
			void turnAxis(const AVec3& axis, spn::RadF rad);
			//! Yaw Pitch Roll指定の回転
			void turnYPR(spn::RadF yaw, spn::RadF pitch, spn::RadF roll);
			//! 差分入力
			void addRot(const AQuat& q);
			//! 補間付き回転
			/*! 3軸の目標距離を合計した物が閾値以下ならtrueを返す */
			bool lerpTurn(const AQuat& q_tgt, float t, float threshold=TURN_THRESHOLD);
			//! Upベクトルをrollが0になるよう補正
			void adjustNoRoll();

			// ---- getter method ----
			const AVec3& getOffset() const;
			const AQuat& getRot() const;
			const AVec3& getScale() const;
			const AMat43& getToWorld() const;
			AMat43 getToLocal() const;
			uint32_t getAccum() const;

			AVec3 getUp() const;
			AVec3 getRight() const;
			AVec3 getDir() const;

			// ---- setter method ----
			void setAll(const AVec3& ofs, const AQuat& q, const AVec3& sc);
			// Scaleに変更を加える
			void setScale(const AVec3& s);
			// Rotationに変更を加える
			void setRot(const AQuat& q);
			void addAxisRot(const AVec3& axis, RadF radf);
			// Positionに変更を加える
			void setOffset(const AVec3& v);
			void addOffset(const AVec3& ad);

			// ---- reference method ----
			AVec3& refOffset();
			AQuat& refRot();
			AVec3& refScale();

			Pose3D lerp(const Pose3D& p1, float t) const;
			Value refValue();
			void apply(const TValue& t);

			Pose3D& operator = (const Pose3D& ps);
			Pose3D& operator = (const TValue& tv);
			Pose3D& operator = (const AMat43& m);
			// ---- compare method ----
			bool operator == (const Pose3D& ps) const;
			bool operator != (const Pose3D& ps) const;
			friend std::ostream& operator << (std::ostream&, const Pose3D&);
	};
	std::ostream& operator << (std::ostream& os, const Pose3D& ps);
}
