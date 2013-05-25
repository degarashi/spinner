#include "pose.hpp"

namespace spn {
	// ------------------------------ Pose2D ------------------------------
	template <bool A>
	Pose2DT<A>::Pose2DT() {
		identity();
	}
	template <bool A>
	Pose2DT<A>::Pose2DT(const VEC2& pos, float ang, const VEC2& sc): Pose2DT() {
		_ofs = pos;
		_angle = ang;
		_scale = sc;
	}
	#define DEF(typ)	template <bool A> BOOST_PP_IDENTITY(typ)() Pose2DT<A>
	DEF(void)::identity() {
		_accum = std::rand();
		_rflag = 0;
		_angle = 0;
		_ofs = _scale = VEC2(0,0);
		_finalMat.identity();
	}
	
	DEF(void)::_refresh() const {
		if(_rflag & (PRF_SCALE|PRF_ROTATE)) {
			float sc[2] = {_scale.x, _scale.y};
			_finalMat = MAT32::Scaling(sc[0], sc[1]);
			_finalMat *= MAT32::Rotation(_angle);
			_finalMat.getRow(2) = _ofs;
		} else if(_rflag & PRF_OFFSET)
			_finalMat.getRow(2) = _ofs;
		_rflag = 0;
	}
	DEF(const typename Pose2DT<A>::MAT32&)::getFinal() const {
		return _finalMat;
	}
	DEF(void)::setAll(const VEC2& ofs, float ang, const VEC2& sc) {
		_rflag = PRF_ALL;
		_ofs = ofs;
		_scale = sc;
		_angle = ang;
		++_accum;
	}
	DEF(const float*)::_getPtr() const {
		return reinterpret_cast<const float*>(&_ofs);
	}
	DEF(void)::setScale(float x, float y) {
		_scale.x = x;
		_scale.y = y;
		_rflag |= PRF_SCALE;
		++_accum;
	}
	DEF(void)::setOfs(float x, float y) {
		_ofs.x = x;
		_ofs.y = y;
		_rflag |= PRF_OFFSET;
		++_accum;
	}
	DEF(void)::setAngle(float ang) {
		_angle = ang;
		_rflag |= PRF_ROTATE;
		++_accum;
	}
	DEF(uint32_t)::getAccum() const {
		return _accum;
	}
	DEF(Pose2DT<A>)::lerp(const Pose2DT& p1, float t) const {
		Pose2DT ret;
		ret._accum = _accum-1;
		ret._ofs = _ofs.l_intp(p1._ofs, t);
		ret._angle = (p1._angle - _angle) * t + _angle;
		ret._scale = _scale.l_intp(p1._scale, t);
		return ret;
	}
	#undef DEF
	template class Pose2DT<false>;
	template class Pose2DT<true>;

	// ------------------------------ Pose3D ------------------------------
	template <bool A>
	Pose3DT<A>::Pose3DT() {
		identity();
	}
	template <bool A>
	Pose3DT<A>::Pose3DT(const VEC3& pos, const QUAT& rot, const VEC3& sc): Pose3DT() {
		_ofs = pos;
		_rot = rot;
		_scale = sc;
	}
	#define DEF(typ)	template <bool A> BOOST_PP_IDENTITY(typ)() Pose3DT<A>
	DEF(void)::identity() {
		++_accum;
		_accum = std::rand();
		_rflag = 0;
		_rot.identity();
		_ofs = Vec3(0,0,0);
		_scale = Vec3(1,1,1);
		_finalMat.identity();
	}

	DEF(void)::_refresh() const {
		if(_rflag & (PRF_SCALE|PRF_ROTATE)) {
			float sc[3] = {_scale.x, _scale.y, _scale.z};
			_finalMat = MAT43::Scaling(sc[0], sc[1], sc[2]);
			_finalMat *= _rot.asMat33();
			_finalMat.getRow(3) = _ofs;
		} else if(_rflag & PRF_OFFSET)
			_finalMat.getRow(3) = _ofs;
		_rflag = 0;
	}
	DEF(const typename Pose3DT<A>::MAT43&)::getFinal() const {
		// 行列の更新
		if(_rflag != 0)
			_refresh();
		return _finalMat;
	}
	DEF(uint32_t)::getAccum() const {
		return _accum;
	}

	DEF(const typename Pose3DT<A>::VEC3&)::getScale() const {
		return _scale;
	}
	DEF(const typename Pose3DT<A>::VEC3&)::getOffset() const {
		return _ofs;
	}
	DEF(const typename Pose3DT<A>::QUAT&)::getRot() const {
		return _rot;
	}
	DEF(const float*)::_getPtr() const {
		return reinterpret_cast<const float*>(&_ofs);
	}
	DEF(void)::setOfs(float x, float y, float z) {
		++_accum;
		_ofs.x = x;
		_ofs.y = y;
		_ofs.z = z;
		_rflag |= PRF_OFFSET;
	}
	DEF(void)::setScale(float x, float y, float z) {
		++_accum;
		_scale.x = x;
		_scale.y = y;
		_scale.z = z;
		_rflag |= PRF_SCALE;
	}
	DEF(void)::setRot(const QUAT& q) {
		++_accum;
		_rot = q;
		_rflag |= PRF_ROTATE;
	}
	DEF(void)::setAll(const VEC3& ofs, const QUAT& q, const VEC3& sc) {
		++_accum;
		_rflag = PRF_ALL;
		_ofs = ofs;
		_rot = q;
		_scale = sc;
	}
	DEF(void)::addAxisRot(const VEC3& axis, float radf) {
		setRot(getRot().rotation(axis, radf));
	}
	DEF(void)::addOfsVec(const VEC3& ad) {
		const Vec3& ofs = getOffset();
		setOfs(ofs.x+ad.x, ofs.y+ad.y, ofs.z+ad.z);
	}
	DEF(void)::setScaleVec(const VEC3& s) {
		setScale(s.x, s.y, s.z);
	}
	DEF(void)::setOfsVec(const VEC3& v) {
		setOfs(v.x, v.y, v.z);
	}
	DEF(typename Pose3DT<A>::VEC3)::getUp() const {
		return getRot().getYAxis();
	}
	DEF(typename Pose3DT<A>::VEC3)::getRight() const {
		return getRot().getXAxis();
	}
	DEF(typename Pose3DT<A>::VEC3)::getDir() const {
		return getRot().getZAxis();
	}
	DEF(Pose3DT<A>)::lerp(const Pose3DT& p1, float t) const {
		Pose3DT ret;
		ret._accum = _accum-1;
		ret._ofs = _ofs.l_intp(p1._ofs, t);
		ret._rot = _rot.lerp(p1._rot, t);
		ret._scale = _scale.l_intp(p1._scale, t);
		return ret;
	}
	template class Pose3DT<false>;
	template class Pose3DT<true>;
}
