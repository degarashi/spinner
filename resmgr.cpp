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
		s_rmList.get(sh.getResID())->increment(sh);
	}
	bool ResMgrBase::Release(SHandle sh) {
		return s_rmList.get(sh.getResID())->release(sh);
	}
	SHandle ResMgrBase::Lock(WHandle wh) {
		return s_rmList.get(wh.getResID())->lock(wh);
	}
	WHandle ResMgrBase::Weak(SHandle sh) {
		return s_rmList.get(sh.getResID())->weak(sh);
	}
}
