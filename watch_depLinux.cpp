#include "watch_depLinux.hpp"
#include <sys/inotify.h>
#include <unistd.h>

namespace spn {
	FNotify_depLinux::FNotify_depLinux() {
		if((fd = inotify_init()) < 0)
			throw std::runtime_error("failed to initialize inotify");
	}
	FNotify_depLinux::~FNotify_depLinux() {
		close(fd);
	}
	FNotify_depLinux::DSC FNotify_depLinux::addWatch(const std::string& path, uint32_t mask) {
		return inotify_add_watch(fd, path.c_str(), ConvertMask(mask));
	}
	void FNotify_depLinux::remWatch(const DSC& dsc) {
		inotify_rm_watch(fd, dsc);
	}
	namespace {
		const static std::pair<FileEvent,int> c_flags[] = {
			{FE_Create, IN_CREATE},
			{FE_Modify, IN_MODIFY},
			{FE_Remove, IN_DELETE},
			{FE_MoveFrom, IN_MOVED_FROM},
			{FE_MoveTo, IN_MOVED_TO},
			{FE_Attribute, IN_ATTRIB}
		};
		constexpr int EVENTSIZE = sizeof(inotify_event);
	}
	uint32_t FNotify_depLinux::ConvertMask(uint32_t mask) {
		uint32_t res = 0;
		for(auto& p : c_flags) {
			if(p.first & mask)
				res |= p.second;
		}
		return res | IN_DELETE_SELF | IN_MOVE_SELF;
	}
	FileEvent FNotify_depLinux::DetectEvent(uint32_t mask) {
		uint32_t res = 0;
		for(auto& p : c_flags) {
			if(p.second & mask)
				return p.first;
		}
		return FE_Invalid;
	}
	void FNotify_depLinux::procEvent(CallbackD cb) {
		char buff[1024];
		auto nread = read(fd, buff, 1024);
		if(nread < 0)
			return;

		int ofs = 0;
		while(ofs < nread) {
			auto* e = reinterpret_cast<inotify_event*>(buff + ofs);
			cb(e->wd, DetectEvent(e->mask), e->name, e->cookie, e->mask&IN_ISDIR);
			ofs += EVENTSIZE + e->len;
		}
	}
}
