#include "watch_depLinux.hpp"
#include <sys/inotify.h>
#include <unistd.h>
#include <mutex>
#include "error.hpp"
#include "common.hpp"

namespace spn {
	FNotify_depLinux::FNotify_depLinux() {
		Assert(Trap, (_fd = inotify_init()) >= 0, "failed to initialize inotify")
		_thread = std::thread(std::packaged_task<void (FNotify_depLinux*)>(ASyncLoop), this);
	}
	FNotify_depLinux::~FNotify_depLinux() {
		if(_thread) {
			int endflag = ~0;
			write(_fd, &endflag, sizeof(int));
			_thread->join();
		}
		close(_fd);
	}
	void FNotify_depLinux::_pushInfo(const inotify_event& e) {
		std::lock_guard<decltype(_mutex)>	lk(_mutex);
		_eventL.push_back(Event{e.wd, DetectEvent(e.mask), e.name, e.cookie, static_cast<bool>(e.mask & IN_ISDIR)});
	}

	FNotify_depLinux::DSC FNotify_depLinux::addWatch(const std::string& path, uint32_t mask) {
		return inotify_add_watch(_fd, path.c_str(), ConvertMask(mask));
	}
	void FNotify_depLinux::remWatch(const DSC& dsc) {
		inotify_rm_watch(_fd, dsc);
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
		std::lock_guard<decltype(_mutex)> lk(_mutex);
		for(auto& e : _eventL)
			cb(e);
		_eventL.clear();
	}
	void FNotify_depLinux::ASyncLoop(FNotify_depLinux* ths) {
		char buff[2048];
		for(;;) {
			auto nread = read(ths->_fd, buff, 2048);
			if(nread < 0)
				continue;

			int ofs = 0;
			while(ofs < nread) {
				auto* e = reinterpret_cast<inotify_event*>(buff + ofs);
				if(e->wd == ~0) {
					// 終了フラグ
					return;
				}
				ths->_pushInfo(*e);
				ofs += EVENTSIZE + e->len;
			}
		}
	}
}
