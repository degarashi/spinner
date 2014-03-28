#include "resmgr.hpp"
#include "path.hpp"

namespace spn {
	none_t none;
	// ------------------ ResMgrBase ------------------
	ResMgrBase::RMList ResMgrBase::s_rmList;
	ResMgrBase::URIHandlerV ResMgrBase::s_handler;
	ResMgrBase::URIHandlerV& ResMgrBase::GetHandler() {
		return s_handler;
	}
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
	void* ResMgrBase::GetPtr(SHandle sh) {
		return GetManager(sh.getResID())->getPtr(sh);
	}
	SHandle ResMgrBase::LoadResource(const URI& uri) {
		UP_Adapt u(s_handler.procHandler(uri));
		if(u) {
			for(auto* p : s_rmList) {
				if(p->canLoad(uri.getExtension().c_str())) {
					SHandle ret = p->loadResource(*u, uri.getLast_utf8());
					Assert(Trap, ret.valid())
					return ret;
				}
			}
		}
		return SHandle();
	}
	SHandle ResMgrBase::loadResource(AdaptStream& ast, const std::string& name) {
		return SHandle();
	}
	bool ResMgrBase::canLoad(const std::string& ext) const {
		return false;
	}
}
