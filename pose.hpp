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
				(((float, _angle), (mutable, uint32_t, _accum)))
			)

			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int /*ver*/) {
				ar & BOOST_SERIALIZATION_NVP(_ofs) & BOOST_SERIALIZATION_NVP(_angle) & BOOST_SERIALIZATION_NVP(_scale);
				if(Archive::is_loading::value) {
					_accum = std::rand();
					_rflag = 0;
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
				float	ang;

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
				float	&ang;

				Value(Pose2D& p);
				~Value();
				Value(const Value& v);
				Value& operator = (const TValue& tv);
				void operator = (const Value& v) = delete;
			};

			Pose2D();
			Pose2D(const Pose2D& p);
			Pose2D(const Vec2& pos, float ang, const Vec2& sc);
			Pose2D(const TValue& tv);
			void identity();

			void setAll(const Vec2& ofs, float ang, const Vec2& sc);
			void setScale(const Vec2& ofs);
			void setAngle(float ang);
			void setOffset(const Vec2& ofs);
			float getAngle() const;
			const Vec2& getOffset() const;
			const Vec2& getScale() const;

			const AMat32& getToWorld() const;
			AMat32 getToLocal() const;
			uint32_t getAccum() const;
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

			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int /*ver*/) {
				ar & BOOST_SERIALIZATION_NVP(_ofs) & BOOST_SERIALIZATION_NVP(_rot) & BOOST_SERIALIZATION_NVP(_scale);
				if(Archive::is_loading::value) {
					_accum = std::rand();
					_rflag = 0;
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

			// 各種変数取得
			const AVec3& getOffset() const;
			const AQuat& getRot() const;
			const AVec3& getScale() const;
			const AMat43& getToWorld() const;
			AMat43 getToLocal() const;
			uint32_t getAccum() const;
			void setAll(const AVec3& ofs, const AQuat& q, const AVec3& sc);
			// Scaleに変更を加える
			void setScale(const AVec3& s);
			// Rotationに変更を加える
			void setRot(const AQuat& q);
			void addAxisRot(const AVec3& axis, float radf);
			// Positionに変更を加える
			void setOffset(const AVec3& v);
			void addOffset(const AVec3& ad);

			AVec3 getUp() const;
			AVec3 getRight() const;
			AVec3 getDir() const;

			Pose3D lerp(const Pose3D& p1, float t) const;
			Value refValue();
			void apply(const TValue& t);

			Pose3D& operator = (const Pose3D& ps);
			Pose3D& operator = (const TValue& tv);
			Pose3D& operator = (const AMat43& m);
			bool operator == (const Pose3D& ps) const;
			bool operator != (const Pose3D& ps) const;
			friend std::ostream& operator << (std::ostream&, const Pose3D&);
	};
	std::ostream& operator << (std::ostream& os, const Pose3D& ps);
}
