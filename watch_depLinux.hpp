#pragma once
#include "watch_common.hpp"
#include <mutex>
#include <thread>
#include <future>
#include "optional.hpp"

struct inotify_event;
namespace spn {
	class FNotify_depLinux {
		public:
			using Event = FNotifyEvent<int>;
		private:
			using OPThread = spn::Optional<std::thread>;
			std::recursive_mutex	_mutex;
			int						_fd,
									_cancelFd[2];
			OPThread				_thread;
			using EventL = std::vector<Event>;
			EventL		_eventL;

			//! ファイル監視用スレッド
			static void ASyncLoop(FNotify_depLinux* ths);
			void _pushInfo(const inotify_event& e);

		public:
			using DSC = int;
			using CallbackD = std::function<void (const Event&)>;

			FNotify_depLinux();
			~FNotify_depLinux();
			DSC addWatch(const std::string& path, uint32_t mask);
			void remWatch(const DSC& dsc);
			void procEvent(CallbackD cb);
			static uint32_t ConvertMask(uint32_t mask);
			static FileEvent DetectEvent(uint32_t mask);
	};
	using FNotifyDep = FNotify_depLinux;
}
