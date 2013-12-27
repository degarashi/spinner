#include "watch.hpp"
#include "dir.hpp"
#include "filetree.hpp"

namespace spn {
	// ------------------------- FEv -------------------------
	FEv::FEv(const SPString& base, void* ud, const std::string& nm, bool bDir):
		basePath(base), udata(ud), name(nm), isDir(bDir) {}
	bool FEv::operator == (const FEv& f) const {
		return 	*basePath == *f.basePath &&
				udata == f.udata &&
				name == f.name &&
				isDir == f.isDir &&
				getType() == f.getType();
	}
	void FEv::print(std::ostream& os) const {
		os << "type: " << getName() << std::endl
		   << "basePath: " << *basePath << std::endl
			  << "udata: " << std::hex << (uintptr_t)udata << std::endl
			  << "name: " << name << std::endl
			  << "isDir: " << std::boolalpha << isDir << std::endl;
	}
	// ------------------------- FEv_Create -------------------------
	FileEvent FEv_Create::getType() const { return FE_Create; }
	const char* FEv_Create::getName() const { return "FEv_Create"; }
	// ------------------------- FEv_Access -------------------------
	FileEvent FEv_Access::getType() const { return FE_Access; }
	const char* FEv_Access::getName() const { return "FEv_Access"; }
	// ------------------------- FEv_Modify -------------------------
	FileEvent FEv_Modify::getType() const { return FE_Modify; }
	const char* FEv_Modify::getName() const { return "FEv_Modify"; }
	// ------------------------- FEv_Remove -------------------------
	FileEvent FEv_Remove::getType() const { return FE_Remove; }
	const char* FEv_Remove::getName() const { return "FEv_Remove"; }
	// ------------------------- FEv_Attr -------------------------
	FileEvent FEv_Attr::getType() const { return FE_Attribute; }
	const char* FEv_Attr::getName() const { return "FEv_Attribute"; }
	// ------------------------- FEv_MoveFrom -------------------------
	FEv_MoveFrom::FEv_MoveFrom(const SPString& base, void* ud, const std::string& name, bool bDir, uint32_t cookieID):
		FEv(base, ud, name, bDir), cookie(cookieID) {}
	bool FEv_MoveFrom::operator == (const FEv& e) const {
		if(static_cast<const FEv&>(*this) == static_cast<const FEv&>(e))
			return cookie == reinterpret_cast<const FEv_MoveFrom&>(e).cookie;
		return false;
	}
	FileEvent FEv_MoveFrom::getType() const { return FE_MoveFrom; }
	const char* FEv_MoveFrom::getName() const { return "FEv_MoveFrom"; }
	void FEv_MoveFrom::print(std::ostream& os) const {
		FEv::print(os);
		os << "cookie: " << cookie << std::endl;
	}
	// ------------------------- FEv_MoveTo -------------------------
	FEv_MoveTo::FEv_MoveTo(const SPString& base, void* ud, const std::string& name, bool bDir, uint32_t cookieID):
		FEv(base, ud, name, bDir), cookie(cookieID) {}

	bool FEv_MoveTo::operator == (const FEv& e) const {
		if(static_cast<const FEv&>(*this) == static_cast<const FEv&>(e))
			return cookie == reinterpret_cast<const FEv_MoveTo&>(e).cookie;
		return false;
	}
	FileEvent FEv_MoveTo::getType() const { return FE_MoveTo; }
	const char* FEv_MoveTo::getName() const { return "FEv_MoveTo"; }
	void FEv_MoveTo::print(std::ostream& os) const {
		FEv::print(os);
		os << "cookie: " << cookie << std::endl;
	}

	// ------------------------- FNotify -------------------------
	void FNotify::addWatch(const std::string& path, uint32_t mask, UData udata) {
		remWatch(path);
		auto dsc = _dep.addWatch(path, mask);
		_path2ent.emplace(path, Ent{dsc, SPString(new std::string(path)), std::move(udata)});
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
		_dep.procEvent([&ntf, this](const FNotifyDep::Event& e){
			auto& ent = _dsc2ent.find(e.dsc)->second;
			auto* ud = ent->udata.get();
			#define MAKE_EVENT(typ) ntf.event(typ(ent->basePath, ud, e.path.c_str(), e.bDir));
			#define MAKE_EVENTM(typ) ntf.event(typ(ent->basePath, ud, e.path.c_str(), e.bDir, e.cookie));
			switch(e.event) {
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
