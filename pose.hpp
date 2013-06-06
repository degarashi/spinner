#pragma once
#include "vector.hpp"
#include "quat.hpp"

namespace spn {
	enum POSERF {
		PRF_OFFSET = 0x01,
		PRF_ROTATE = 0x02,
		PRF_SCALE = 0x04,
		PRF_ALL = 0x07
	};
	//! 2次元姿勢クラス
	/*! 変換適用順序は[拡縮][回転][平行移動] */
	class Pose2D {
		Vec2&			_ofs;
		Vec2&			_scale;

		mutable AMat32		_finalMat;
		mutable uint32_t	_rflag;
		float				_angle;
		uint32_t			_accum;

		void _refresh() const;
		protected:
			const float* _getPtr() const;
		public:
			Pose2D();
			Pose2D(const Vec2& pos, float ang, const Vec2& sc);
			void identity();

			void setAll(const Vec2& ofs, float ang, const Vec2& sc);
			void setScale(float x, float y);
			void setAngle(float ang);
			void setOfs(float x, float y);

			const AMat32& getFinal() const;
			uint32_t getAccum() const;
			Pose2D lerp(const Pose2D& p1, float t) const;
	};

	//! 3次元姿勢クラス
	class Pose3D {
		AVec3			_ofs;
		AQuat			_rot;
		AVec3			_scale;

		mutable AMat43		_finalMat;
		mutable uint32_t	_rflag;
		uint32_t&			_accum;

		void _refresh() const;

		protected:
			const float* _getPtr() const;
		public:
			Pose3D();
			Pose3D(const AVec3& pos, const AQuat& rot, const AVec3& sc);
			void identity();

			// 各種変数取得
			const AVec3& getOffset() const;
			const AQuat& getRot() const;
			const AVec3& getScale() const;
			const AMat43& getFinal() const;
			uint32_t getAccum() const;
			void setAll(const AVec3& ofs, const AQuat& q, const AVec3& sc);
			// Scaleに変更を加える
			void setScale(float x, float y, float z);
			void setScaleVec(const AVec3& s);
			// Rotationに変更を加える
			void setRot(const AQuat& q);
			void addAxisRot(const AVec3& axis, float radf);
			// Positionに変更を加える
			void setOfsVec(const AVec3& v);
			void setOfs(float x, float y, float z);
			void addOfsVec(const AVec3& ad);

			AVec3 getUp() const;
			AVec3 getRight() const;
			AVec3 getDir() const;

			Pose3D lerp(const Pose3D& p1, float t) const;
	};
}
