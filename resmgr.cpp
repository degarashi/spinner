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
	const std::string& ResMgrBase::getResourceName(SHandle /*sh*/) const {
		static std::string emptyName;
		return emptyName;
	}
	LHandle ResMgrBase::LoadResource(const URI& uri) {
		UP_Adapt u(s_handler.procHandler(uri));
		auto name = uri.getLast_utf8(),
			ext = uri.getExtension();
		if(u) {
			for(auto* p : s_rmList) {
				if(LHandle ret = p->loadResource(*u, uri))
					return ret;
			}
		}
		return LHandle();
	}
	LHandle ResMgrBase::loadResource(AdaptStream& /*ast*/, const URI& /*uri*/) {
		return LHandle();
	}
}
