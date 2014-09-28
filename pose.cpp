#include "pose.hpp"
#include <cstdlib>

namespace spn {
	// ------------------------------ Pose2D ------------------------------
	Pose2D::Value::Value(Pose2D& p): _pose(p), ofs(p._ofs), scale(p._scale), ang(p._angle) {}
	Pose2D::Value::~Value() { _pose._setAsChanged(); }
	Pose2D::Value::Value(const Pose2D::Value& v): _pose(v._pose), ofs(v.ofs), scale(v.scale), ang(v.ang) {}
	Pose2D::Value& Pose2D::Value::operator = (const TValue& tv) {
		ofs = tv.ofs;
		scale = tv.scale;
		ang = tv.ang;
		return *this;
	}
	Pose2D::TValue::TValue(const Value& v): ofs(v.ofs), scale(v.scale), ang(v.ang) {}
	Pose2D::TValue& Pose2D::TValue::operator = (const Value& v) {
		ofs = v.ofs;
		scale = v.scale;
		ang = v.ang;
		return *this;
	}
	Pose2D::TValue::TValue(const Pose2D& p): ofs(p.getOffset()), scale(p.getScale()), ang(p.getAngle()) {}
	Pose2D::TValue& Pose2D::TValue::operator = (const Pose2D& p) {
		ofs = p.getOffset();
		scale = p.getScale();
		ang = p.getAngle();
		return *this;
	}

	Pose2D::Pose2D() {
		identity();
	}
	Pose2D::Pose2D(const Pose2D& p) {
		// trivialなctorばかりなのでmemcpyで済ませる
		std::memcpy(this, &p, sizeof(p));
	}
	Pose2D::Pose2D(const Vec2& pos, RadF ang, const Vec2& sc): Pose2D() {
		_ofs = pos;
		_angle = ang;
		_scale = sc;
	}
	Pose2D::Pose2D(const TValue& tv) {
		_ofs = tv.ofs;
		_scale = tv.scale;
		_angle = tv.ang;
		_accum = std::rand();
		_rflag = PRF_ALL;
	}

	void Pose2D::identity() {
		_finalMat.identity();
		_accum = std::rand();
		_angle->set(0);
		_rflag = 0;
		_ofs = Vec2(0);
		_scale = Vec2(1);
	}
	void Pose2D::_setAsChanged() {
		_rflag = PRF_ALL;
		++_accum;
	}
	const Vec2& Pose2D::getOffset() const {
		return _ofs;
	}
	RadF Pose2D::getAngle() const {
		return _angle;
	}
	const Vec2& Pose2D::getScale() const {
		return _scale;
	}
	bool Pose2D::operator == (const Pose2D& ps) const {
		return _ofs == ps._ofs &&
				_angle == ps._angle &&
				_scale == ps._scale;
	}
	bool Pose2D::operator != (const Pose2D& ps) const {
		return !(this->operator == (ps));
	}
	Pose2D& Pose2D::operator = (const Pose2D& ps) {
		std::memcpy(this, &ps, sizeof(ps));
		return *this;
	}
	Pose2D& Pose2D::operator = (const TValue& tv) {
		_ofs = tv.ofs;
		_scale = tv.scale;
		_angle = tv.ang;
		_setAsChanged();
		return *this;
	}

	void Pose2D::_refresh() const {
		if(_rflag & (PRF_SCALE|PRF_ROTATE)) {
			_finalMat = AMat32::Scaling(_scale->x, _scale->y);
			_finalMat = _finalMat.convertA33() * AMat32::Rotation(_angle);
			_finalMat.getRow(2) = _ofs;
		} else if(_rflag & PRF_OFFSET)
			_finalMat.getRow(2) = _ofs;
		_rflag = 0;
	}
	const AMat32& Pose2D::getToWorld() const {
		_refresh();
		return _finalMat;
	}
	AMat32 Pose2D::getToLocal() const {
		AMat33 m = getToWorld().convertA33();
		m.invert();
		// AMat32とAMat33の使用メモリ領域は同じ
		return reinterpret_cast<const AMat32&>(m);
	}
	void Pose2D::setAll(const Vec2& ofs, RadF ang, const Vec2& sc) {
		_rflag = PRF_ALL;
		_ofs = ofs;
		_scale = sc;
		_angle = ang;
		++_accum;
	}
	const float* Pose2D::_getPtr() const {
		return reinterpret_cast<const float*>(&_ofs);
	}
	void Pose2D::setScale(const Vec2& s) {
		_scale->x = s.x;
		_scale->y = s.y;
		_rflag |= PRF_SCALE;
		++_accum;
	}
	void Pose2D::setOffset(const Vec2& ofs) {
		_ofs->x = ofs.x;
		_ofs->y = ofs.y;
		_rflag |= PRF_OFFSET;
		++_accum;
	}
	void Pose2D::setAngle(RadF ang) {
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
		ret._ofs = _ofs->l_intp((const Vec2&)p1._ofs, t);
		ret._angle = (*p1._angle - *_angle) * t + (*_angle);
		ret._scale = _scale->l_intp((const Vec2&)p1._scale, t);
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
	std::ostream& operator << (std::ostream& os, const Pose2D& ps) {
		return os << "Pose2D [ offset: " << ps._ofs << std::endl
				<< "angle: " << *ps._angle << std::endl
				<< "scale: " << *ps._scale << ']';
	}

	// ------------------------------ Pose3D ------------------------------
	Pose3D::Value::Value(Pose3D& p): _pose(p), ofs(p._ofs), scale(p._scale), rot(p._rot) {}
	Pose3D::Value::~Value() { _pose._setAsChanged(); }
	Pose3D::Value::Value(const Value& v): _pose(v._pose), ofs(v.ofs), scale(v.scale), rot(v.rot) {}
	bool Pose3D::operator == (const Pose3D& ps) const {
		return _ofs == ps._ofs &&
				_rot == ps._rot &&
				_scale == ps._scale;
	}
	bool Pose3D::operator != (const Pose3D& ps) const {
		return !(this->operator == (ps));
	}
	Pose3D::Value& Pose3D::Value::operator = (const TValue& tv) {
		ofs = tv.ofs;
		scale = tv.scale;
		rot = tv.rot;
		return *this;
	}
	Pose3D::TValue::TValue(const Value& v): ofs(v.ofs), scale(v.scale), rot(v.rot) {}
	Pose3D::TValue& Pose3D::TValue::operator = (const Value& v) {
		ofs = v.ofs;
		scale = v.scale;
		rot = v.rot;
		return *this;
	}
	Pose3D::TValue::TValue(const Pose3D& p): ofs(p.getOffset()), scale(p.getScale()), rot(p.getRot()) {}
	Pose3D::TValue& Pose3D::TValue::operator = (const Pose3D& p) {
		ofs = p.getOffset();
		scale = p.getScale();
		rot = p.getRot();
		return *this;
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
	Pose3D::Pose3D(const TValue& tv): _ofs(tv.ofs), _rot(tv.rot), _scale(tv.scale) {
		_rflag = PRF_ALL;
	}
	Pose3D::Pose3D(const AMat43& m) {
		auto ap = AffineParts::Decomp(m);
		_ofs = ap.offset;
		_rot = ap.rotation;
		_scale = ap.scale;
		_rflag = PRF_ALL;
	}

	void Pose3D::identity() {
		_rflag = 0;
		_accum = std::rand();
		_finalMat.identity();
		_rot.identity();
		_ofs = AVec3(0,0,0);
		_scale = AVec3(1,1,1);
		_setAsChanged();
	}
	void Pose3D::_setAsChanged() {
		_rflag = PRF_ALL;
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
	const AMat43& Pose3D::getToWorld() const {
		// 行列の更新
		if(_rflag != 0)
			_refresh();
		return _finalMat;
	}
	AMat43 Pose3D::getToLocal() const {
		AMat44 toW = getToWorld().convertA44();
		toW.invert();
		// AMat43とAMat44の使用メモリ領域は同じ
		return reinterpret_cast<const AMat43&>(toW);
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
	void Pose3D::setOffset(const AVec3& v) {
		++_accum;
		_ofs.x = v.x;
		_ofs.y = v.y;
		_ofs.z = v.z;
		_rflag |= PRF_OFFSET;
	}
	void Pose3D::setScale(const AVec3& s) {
		++_accum;
		_scale.x = s.x;
		_scale.y = s.y;
		_scale.z = s.z;
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
	void Pose3D::addAxisRot(const AVec3& axis, RadF radf) {
		setRot(getRot().rotation(axis, radf));
	}
	void Pose3D::addOffset(const AVec3& ad) {
		const AVec3& ofs = getOffset();
		setOffset(ad + ofs);
	}
	AVec3 Pose3D::getUp() const {
		return getRot().getUp();
	}
	AVec3 Pose3D::getRight() const {
		return getRot().getRight();
	}
	AVec3 Pose3D::getDir() const {
		return getRot().getDir();
	}
	Pose3D Pose3D::lerp(const Pose3D& p1, float t) const {
		Pose3D ret;
		ret._accum = _accum-1;
		ret._ofs = _ofs.l_intp(p1._ofs, t);
		ret._rot = _rot.slerp(p1._rot, t);
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
	Pose3D& Pose3D::operator = (const AMat43& m) {
		return *this = Pose3D(m);
	}
	Pose3D& Pose3D::operator = (const Pose3D& ps) {
		std::memcpy(this, &ps, sizeof(ps));
		return *this;
	}
	Pose3D& Pose3D::operator = (const TValue& tv) {
		_ofs = tv.ofs;
		_rot = tv.rot;
		_scale = tv.scale;
		_setAsChanged();
		return *this;
	}
	std::ostream& operator << (std::ostream& os, const Pose3D& ps) {
		return os << "Pose3D [ offset:" << ps._ofs << std::endl
				<< "rotation: " << ps._rot << std::endl
				<< "scale: " << ps._scale << ']';
	}
}
