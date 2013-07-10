#include "resmgr.hpp"

namespace spn {
	// ------------------ ResMgrBase ------------------
	ResMgrBase::RMList ResMgrBase::s_rmList;

	int ResMgrBase::_addManager(ResMgrBase* p) {
		assert(std::find(s_rmList.begin(), s_rmList.end(), p) == s_rmList.end());
		int idx = s_rmList.size();
		s_rmList.push_back(p);
		return idx;
	}
	void ResMgrBase::Increment(SHandle sh) {
		s_rmList[sh.getResID()]->increment(sh);
	}
	bool ResMgrBase::Release(SHandle sh) {
		return s_rmList[sh.getResID()]->release(sh);
	}
	SHandle ResMgrBase::Lock(WHandle wh) {
		return s_rmList[wh.getResID()]->lock(wh);
	}
	WHandle ResMgrBase::Weak(SHandle sh) {
		return s_rmList[sh.getResID()]->weak(sh);
	}
}
