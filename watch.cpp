#include "watch.hpp"

namespace spn {
	void FNotify::addWatch(const std::string& path, uint32_t mask, UPVoid udata) {
		remWatch(path);
		auto dsc = _dep.addWatch(path, mask);
		_path2ent.emplace(path, Ent{dsc, path, std::move(udata)});
		_dsc2ent.emplace(dsc, &_path2ent.at(path));
	}
	void FNotify::remWatch(const std::string& path) {
		auto itr = _path2ent.find(path);
		if(itr != _path2ent.end()) {
			_dep.remWatch(itr->second.dsc);
			_dsc2ent.erase(_dsc2ent.find(itr->second.dsc));
			_path2ent.erase(itr);
		}
	}
	void FNotify::procEvent(FRecvNotify& ntf) {
		_dep.procEvent([&ntf, this](const DSC& dsc, FileEvent evtype, const char* name, uint32_t cookieID, bool isDir){
			auto& ent = _dsc2ent.find(dsc)->second;
			auto* ud = ent->udata.get();
			#define MAKE_EVENT(typ) ntf.event(typ(ent->basePath, ud, name, isDir));
			#define MAKE_EVENTM(typ) ntf.event(typ(ent->basePath, ud, name, isDir, cookieID));
			switch(evtype) {
				case FE_Create:
					MAKE_EVENT(FEv_Create) break;
				case FE_Modify:
					MAKE_EVENT(FEv_Modify) break;
				case FE_Remove:
					MAKE_EVENT(FEv_Remove) break;
				case FE_MoveFrom:
					MAKE_EVENTM(FEv_MoveFrom) break;
				case FE_MoveTo:
					MAKE_EVENTM(FEv_MoveTo) break;
				default: break;
			}
			#undef MAKE_EVENTM
			#undef MAKE_EVENT
		});
	}
}
