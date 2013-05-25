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
	template <bool A>
	class Pose2DT {
		using VEC2 = VecT<2,A>;
		using MAT32 = MatT<3,2,A>;
		
		VEC2			_ofs;
		uint32_t		_accum;
		float			_angle;
		VEC2			_scale;
		
		mutable MAT32		_finalMat;
		mutable uint32_t	_rflag;
		void _refresh() const;
		protected:
			const float* _getPtr() const;
		public:
			Pose2DT();
			Pose2DT(const VEC2& pos, float ang, const VEC2& sc);
			void identity();

			void setAll(const VEC2& ofs, float ang, const VEC2& sc);
			void setScale(float x, float y);
			void setAngle(float ang);
			void setOfs(float x, float y);
		
			const MAT32& getFinal() const;
			uint32_t getAccum() const;
			Pose2DT lerp(const Pose2DT& p1, float t) const;
	};
	using Pose2D = Pose2DT<false>;
	using APose2D = Pose2DT<true>;

	//! 3次元姿勢クラス
	template <bool A>
	class Pose3DT {
		using VEC3 = VecT<3,A>;
		using QUAT = QuatT<A>;
		using MAT43 = MatT<4,3,A>;
		
		VEC3			_ofs;
		uint32_t		_accum;
		QUAT			_rot;
		VEC3			_scale;
		
		mutable MAT43		_finalMat;
		mutable uint32_t	_rflag;
		void _refresh() const;
		
		protected:
			const float* _getPtr() const;
		public:
			Pose3DT();
			Pose3DT(const VEC3& pos, const QUAT& rot, const VEC3& sc);
			void identity();

			// 各種変数取得
			const VEC3& getOffset() const;
			const QUAT& getRot() const;
			const VEC3& getScale() const;
			const MAT43& getFinal() const;
			uint32_t getAccum() const;
			void setAll(const VEC3& ofs, const QUAT& q, const VEC3& sc);
			// Scaleに変更を加える
			void setScale(float x, float y, float z);
			void setScaleVec(const VEC3& s);
			// Rotationに変更を加える
			void setRot(const QUAT& q);
			void addAxisRot(const VEC3& axis, float radf);
			// Positionに変更を加える
			void setOfsVec(const VEC3& v);
			void setOfs(float x, float y, float z);
			void addOfsVec(const VEC3& ad);

			VEC3 getUp() const;
			VEC3 getRight() const;
			VEC3 getDir() const;

			Pose3DT lerp(const Pose3DT& p1, float t) const;
	};
	using Pose3D = Pose3DT<false>;
	using APose3D = Pose3DT<true>;
}
