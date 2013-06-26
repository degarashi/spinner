#include "pose.hpp"

namespace spn {
	// ------------------------------ Pose2D ------------------------------
	Pose2D::Value::Value(Pose2D& p): _pose(p), ofs(p._ofs), scale(p._scale), ang(p._angle) {}
	Pose2D::Value::~Value() { _pose._setAsChanged(); }

	Pose2D::Pose2D(): _finalMat() {
		identity();
	}
	Pose2D::Pose2D(const Pose2D& p): _finalMat() {
		// trivialなctorばかりなのでmemcpyで済ませる
		std::memcpy(this, &p, sizeof(p));
	}
	Pose2D::Pose2D(const Vec2& pos, float ang, const Vec2& sc): Pose2D() {
		_ofs = pos;
		_angle = ang;
		_scale = sc;
	}
	void Pose2D::identity() {
		_finalMat.identity();
		_accum = std::rand();
		_angle = 0;
		_ofs = Vec2(0);
		_scale = Vec2(1);
		_setAsChanged();
	}
	void Pose2D::_setAsChanged() {
		_rflag = PRF_ALL;
	}
	const Vec2& Pose2D::getOffset() const {
		return _ofs;
	}
	float Pose2D::getAngle() const {
		return _angle;
	}

	void Pose2D::_refresh() const {
		if(_rflag & (PRF_SCALE|PRF_ROTATE)) {
			_finalMat = AMat32::Scaling(_scale.x, _scale.y);
			_finalMat *= AMat32::Rotation(_angle);
			_finalMat.getRow(2) = _ofs;
		} else if(_rflag & PRF_OFFSET)
			_finalMat.getRow(2) = _ofs;
		_rflag = 0;
	}
	const AMat32& Pose2D::getFinal() const {
		_refresh();
		return _finalMat;
	}
	void Pose2D::setAll(const Vec2& ofs, float ang, const Vec2& sc) {
		_rflag = PRF_ALL;
		_ofs = ofs;
		_scale = sc;
		_angle = ang;
		++_accum;
	}
	const float* Pose2D::_getPtr() const {
		return reinterpret_cast<const float*>(&_ofs);
	}
	void Pose2D::setScale(const Vec2& ofs) {
		setScale(ofs.x, ofs.y);
	}
	void Pose2D::setScale(float x, float y) {
		_scale.x = x;
		_scale.y = y;
		_rflag |= PRF_SCALE;
		++_accum;
	}
	void Pose2D::setOfs(const Vec2& ofs) {
		setOfs(ofs.x, ofs.y);
	}
	void Pose2D::setOfs(float x, float y) {
		_ofs.x = x;
		_ofs.y = y;
		_rflag |= PRF_OFFSET;
		++_accum;
	}
	void Pose2D::setAngle(float ang) {
		_angle = ang;
		_rflag |= PRF_ROTATE;
		++_accum;
	}
	uint32_t Pose2D::getAccum() const {
		return _accum;
	}
	Pose2D Pose2D::lerp(const Pose2D& p1, float t) const {
		Pose2D ret;
		ret._accum = _accum-1;
		ret._ofs = _ofs.l_intp(p1._ofs, t);
		ret._angle = (p1._angle - _angle) * t + _angle;
		ret._scale = _scale.l_intp(p1._scale, t);
		return ret;
	}
	Pose2D::Value Pose2D::refValue() {
		return Value(*this);
	}
	void Pose2D::apply(const TValue& t) {
		_ofs = t.ofs;
		_scale = t.scale;
		_angle = t.ang;
		_setAsChanged();
	}

	// ------------------------------ Pose3D ------------------------------
	Pose3D::Value::Value(Pose3D& p): _pose(p), ofs(p._ofs), scale(p._scale), rot(p._rot) {}
	Pose3D::Value::~Value() {
		_pose._setAsChanged();
	}

	Pose3D::Pose3D(): _finalMat() {
		identity();
	}
	Pose3D::Pose3D(const Pose3D& p): _finalMat() {
		// trivialなctorばかりなのでmemcpyで済ませる
		std::memcpy(this, &p, sizeof(p));
	}
	Pose3D::Pose3D(const AVec3& pos, const AQuat& rot, const AVec3& sc): Pose3D() {
		_ofs = pos;
		_rot = rot;
		_scale = sc;
	}
	void Pose3D::identity() {
		_accum = std::rand();
		_finalMat.identity();
		_rot.identity();
		_ofs = AVec3(0,0,0);
		_scale = AVec3(1,1,1);
		_setAsChanged();
	}
	void Pose3D::_setAsChanged() {
		_rflag = 0;
		++_accum;
	}

	void Pose3D::_refresh() const {
		if(_rflag & (PRF_SCALE|PRF_ROTATE)) {
			float sc[3] = {_scale.x, _scale.y, _scale.z};
			_finalMat = AMat43::Scaling(sc[0], sc[1], sc[2]);
			_finalMat *= _rot.asMat33();
			_finalMat.getRow(3) = _ofs;
		} else if(_rflag & PRF_OFFSET)
			_finalMat.getRow(3) = _ofs;
		_rflag = 0;
	}
	const AMat43& Pose3D::getFinal() const {
		// 行列の更新
		if(_rflag != 0)
			_refresh();
		return _finalMat;
	}
	uint32_t Pose3D::getAccum() const {
		return _accum;
	}

	const AVec3& Pose3D::getScale() const {
		return _scale;
	}
	const AVec3& Pose3D::getOffset() const {
		return _ofs;
	}
	const AQuat& Pose3D::getRot() const {
		return _rot;
	}
	const float* Pose3D::_getPtr() const {
		return reinterpret_cast<const float*>(&_ofs);
	}
	void Pose3D::setOfs(float x, float y, float z) {
		++_accum;
		_ofs.x = x;
		_ofs.y = y;
		_ofs.z = z;
		_rflag |= PRF_OFFSET;
	}
	void Pose3D::setScale(float x, float y, float z) {
		++_accum;
		_scale.x = x;
		_scale.y = y;
		_scale.z = z;
		_rflag |= PRF_SCALE;
	}
	void Pose3D::setRot(const AQuat& q) {
		++_accum;
		_rot = q;
		_rflag |= PRF_ROTATE;
	}
	void Pose3D::setAll(const AVec3& ofs, const AQuat& q, const AVec3& sc) {
		++_accum;
		_rflag = PRF_ALL;
		_ofs = ofs;
		_rot = q;
		_scale = sc;
	}
	void Pose3D::addAxisRot(const AVec3& axis, float radf) {
		setRot(getRot().rotation(axis, radf));
	}
	void Pose3D::addOfsVec(const AVec3& ad) {
		const AVec3& ofs = getOffset();
		setOfs(ofs.x+ad.x, ofs.y+ad.y, ofs.z+ad.z);
	}
	void Pose3D::setScaleVec(const AVec3& s) {
		setScale(s.x, s.y, s.z);
	}
	void Pose3D::setOfsVec(const AVec3& v) {
		setOfs(v.x, v.y, v.z);
	}
	AVec3 Pose3D::getUp() const {
		return getRot().getYAxis();
	}
	AVec3 Pose3D::getRight() const {
		return getRot().getXAxis();
	}
	AVec3 Pose3D::getDir() const {
		return getRot().getZAxis();
	}
	Pose3D Pose3D::lerp(const Pose3D& p1, float t) const {
		Pose3D ret;
		ret._accum = _accum-1;
		ret._ofs = _ofs.l_intp(p1._ofs, t);
		ret._rot = _rot.lerp(p1._rot, t);
		ret._scale = _scale.l_intp(p1._scale, t);
		return ret;
	}
	Pose3D::Value Pose3D::refValue() {
		return Value(*this);
	}
	void Pose3D::apply(const TValue& t) {
		_ofs = t.ofs;
		_scale = t.scale;
		_rot = t.rot;
		_setAsChanged();
	}
}
