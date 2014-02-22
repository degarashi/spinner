#include "resmgr.hpp"

namespace spn {
	none_t none;
	// ------------------ ResMgrBase ------------------
	ResMgrBase::RMList ResMgrBase::s_rmList;

	int ResMgrBase::_addManager(ResMgrBase* p) {
		assert(std::find(s_rmList.begin(), s_rmList.end(), p) == s_rmList.end());
		return s_rmList.add(p);
	}
	void ResMgrBase::_remManager(int id) {
		s_rmList.rem(id);
	}
	void ResMgrBase::Increment(SHandle sh) {
		GetManager(sh.getResID())->increment(sh);
	}
	bool ResMgrBase::Release(SHandle sh) {
		return GetManager(sh.getResID())->release(sh);
	}
	uint32_t ResMgrBase::Count(SHandle sh) {
		return GetManager(sh.getResID())->count(sh);
	}
	SHandle ResMgrBase::Lock(WHandle wh) {
		return GetManager(wh.getResID())->lock(wh);
	}
	WHandle ResMgrBase::Weak(SHandle sh) {
		return GetManager(sh.getResID())->weak(sh);
	}
	ResMgrBase* ResMgrBase::GetManager(int rID) {
		return s_rmList.get(rID);
	}
}
