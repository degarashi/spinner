#pragma once
#include "dir_depWin.hpp"
#include "watch_common.hpp"
#include "filetree.hpp"

namespace spn {
	class CriticalSection {
		CRITICAL_SECTION _cs;
		public:
			CriticalSection();
			~CriticalSection();
			LPCRITICAL_SECTION get();
			LPCRITICAL_SECTION operator * ();
			void enter();
			void leave();
	};

	class FNotify_depWin {
		public:
			using DSC = HANDLE;
			using Event = FNotifyEvent<DSC>;
			using CallbackD = std::function<void (const Event&)>;
		private:
			UPWinH			_hEvent,		//!< 本スレ -> イベントスレ通知用
							_hThread;		//!< イベント処理用スレッドハンドル
			enum class EventID {
				Add,
				Rem,
				Exit
			} _eventID;
			DSC	_eventArg;

			struct Entry {
				constexpr static int BUFFSIZE = 1024*8;
				UPWinH		hDir,
							hEvent;
				OVERLAPPED	ovl;
				uint8_t		buff[BUFFSIZE];
				SPString	basePath;

				Entry() = default;
				Entry(Entry&& e);
			};
			using PathToEnt = std::unordered_map<std::string, Entry>;
			using DSCToEntP = std::unordered_map<DSC, Entry*>;
			PathToEnt		_pathToEnt;
			DSCToEntP		_dscToEntP;
			CriticalSection	_csection;
			using EventL = std::vector<Event>;
			EventL			_eventL;

			static unsigned int __stdcall ASyncLoop(void* ths_p);
			void _pushInfo(Entry& ent, FILE_NOTIFY_INFORMATION& pData, uint32_t cookie);
			static void _SetWaitNotify(Entry& ent);

		public:
			FNotify_depWin();
			~FNotify_depWin();
			DSC addWatch(const std::string& path, uint32_t mask);
			void remWatch(DSC dsc);
			void procEvent(CallbackD cb);

			static uint32_t ConvertMask(uint32_t mask);
			static FileEvent DetectEvent(uint32_t mask);
	};
	using FNotifyDep = FNotify_depWin;
}
