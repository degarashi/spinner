#include "watch_depLinux.hpp"
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>
#include <mutex>
#include "error.hpp"
#include "common.hpp"

namespace spn {
	FNotify_depLinux::FNotify_depLinux() {
		Assert(Trap, (_fd = inotify_init()) >= 0, "failed to initialize inotify")
		_thread = boost::thread(ASyncLoop, this);
		::pipe(_cancelFd);
	}
	FNotify_depLinux::~FNotify_depLinux() {
		if(_thread) {
			int endflag = ~0;
			write(_cancelFd[1], &endflag, sizeof(int));
			_thread->join();
		}
		close(_fd);
		close(_cancelFd[0]);
		close(_cancelFd[1]);
	}
	void FNotify_depLinux::_pushInfo(const inotify_event& e) {
		boost::lock_guard<decltype(_mutex)>	lk(_mutex);
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
			{FE_Attribute, IN_ATTRIB},
			{FE_Access, IN_ACCESS}
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
		for(auto& p : c_flags) {
			if(p.second & mask)
				return p.first;
		}
		return FE_Invalid;
	}
	void FNotify_depLinux::procEvent(CallbackD cb) {
		boost::lock_guard<decltype(_mutex)> lk(_mutex);
		for(auto& e : _eventL)
			cb(e);
		_eventL.clear();
	}
	void FNotify_depLinux::ASyncLoop(FNotify_depLinux* ths) {
		char buff[2048];
		for(;;) {
			::timeval tm;
			tm.tv_sec = 1;
			tm.tv_usec = 0;
			::fd_set fds;
			FD_ZERO(&fds);
			FD_SET(ths->_fd, &fds);
			FD_SET(ths->_cancelFd[0], &fds);
			int ret = ::select(std::max(ths->_fd, ths->_cancelFd[0])+1, &fds, nullptr, nullptr, &tm);
			if(ret < 0)
				break;
			else if(ret == 0) {
				// Timeout
			} else {
				if(FD_ISSET(ths->_fd, &fds)) {
					// inotify
					auto nread = read(ths->_fd, buff, 2048);
					if(nread < 0)
						break;
					int ofs = 0;
					while(ofs < nread) {
						auto* e = reinterpret_cast<inotify_event*>(buff + ofs);
						ths->_pushInfo(*e);
						ofs += EVENTSIZE + e->len;
					}
				} else {
					// end
					AssertP(Trap, FD_ISSET(ths->_cancelFd[0], &fds))
					break;
				}
			}
		}
	}
}
