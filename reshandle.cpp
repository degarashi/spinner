#include "resmgr.hpp"

namespace spn {
	// ------------------ SHandle ------------------
	SHandle::SHandle(): _value(~0) {}
	SHandle::SHandle(VWord w): _value(w) {}
	#ifdef DEBUG
		SHandle::VWord SHandle::getMagic() const {
			return _value.at<Value::MAGIC>();
		}
		SHandle::SHandle(int idx, int resID, int mag) {
			_value.at<Value::MAGIC>() = mag;
	#else
		SHandle::SHandle(int idx, int resID) {
	#endif
			_value.at<Value::INDEX>() = idx;
			_value.at<Value::RESID>() = resID;
		}
	void SHandle::setNull() {
		_value = ~0;
	}
	SHandle::VWord SHandle::getIndex() const {
		return _value.at<Value::INDEX>();
	}
	SHandle::VWord SHandle::getResID() const {
		return _value.at<Value::RESID>();
	}
	SHandle::VWord SHandle::getValue() const {
		return _value.value();
	}
	bool SHandle::valid() const {
		return _value.value() != VWord(~0);
	}
	SHandle::operator bool () const {
		return valid();
	}
	bool SHandle::operator == (const SHandle& sh) const {
		return _value.value() == sh._value.value();
	}
	bool SHandle::operator != (const SHandle& sh) const {
		return !(this->operator == (sh));
	}
	bool SHandle::operator < (const SHandle& sh) const {
		return _value.value() < sh._value.value();
	}
	void SHandle::swap(SHandle& sh) noexcept {
		std::swap(_value, sh._value);
	}
	bool SHandle::release() {
		return ResMgrBase::Release(*this);
	}
	void SHandle::increment() {
		ResMgrBase::Increment(*this);
	}
	uint32_t SHandle::count() const {
		return ResMgrBase::Count(*this);
	}
	WHandle SHandle::weak() const {
		return ResMgrBase::Weak(*this);
	}
	ResMgrBase* SHandle::getManager() {
		return ResMgrBase::GetManager(getResID());
	}
	const ResMgrBase* SHandle::getManager() const {
		return ResMgrBase::GetManager(getResID());
	}
	const std::string& SHandle::getResourceName() const {
		return getManager()->getResourceName(*this);
	}

	// ------------------ WHandle ------------------
	WHandle::WHandle(): _value(~0) {}
	WHandle::WHandle(VWord w): _value(w) {}
	WHandle::WHandle(int idx, int resID, VWord mag) {
		_value.at<Value::INDEX>() = idx;
		_value.at<Value::RESID>() = resID;
		_value.at<Value::MAGIC>() = mag;
	}
	void WHandle::setNull() {
		_value = ~0;
	}
	WHandle::VWord WHandle::getIndex() const {
		return _value.at<Value::INDEX>();
	}
	WHandle::VWord WHandle::getResID() const {
		return _value.at<Value::RESID>();
	}
	WHandle::VWord WHandle::getMagic() const {
		return _value.at<Value::MAGIC>();
	}
	WHandle::VWord WHandle::getValue() const {
		return _value.value();
	}
	bool WHandle::valid() const {
		return _value.value() != VWord(~0);
	}
	WHandle::operator bool () const {
		return valid();
	}
	bool WHandle::operator == (const WHandle& wh) const {
		return _value.value() == wh._value.value();
	}
	bool WHandle::operator != (const WHandle& wh) const {
		return !(this->operator == (wh));
	}
	bool WHandle::operator < (const WHandle& wh) const {
		return _value.value() < wh._value.value();
	}
	void WHandle::swap(WHandle& wh) noexcept {
		std::swap(_value, wh._value);
	}
	ResMgrBase* WHandle::getManager() {
		return ResMgrBase::GetManager(getResID());
	}
	const ResMgrBase* WHandle::getManager() const {
		return ResMgrBase::GetManager(getResID());
	}
}
