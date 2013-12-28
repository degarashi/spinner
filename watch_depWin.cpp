#include "watch_depWin.hpp"
#include <process.h>

namespace spn {
	CriticalSection::CriticalSection() {
		InitializeCriticalSection(&_cs);
	}
	CriticalSection::~CriticalSection() {
		DeleteCriticalSection(&_cs);
	}
	LPCRITICAL_SECTION CriticalSection::get() {
		return &_cs;
	}
	LPCRITICAL_SECTION CriticalSection::operator * () {
		return &_cs;
	}
	void CriticalSection::enter() {
		EnterCriticalSection(&_cs);
	}
	void CriticalSection::leave() {
		LeaveCriticalSection(&_cs);
	}

	FNotify_depWin::Entry::Entry(Entry&& e): hDir(std::move(e.hDir)), hEvent(std::move(e.hEvent)),
		ovl(e.ovl), basePath(std::move(e.basePath))
	{
		std::memcpy(buff, e.buff, BUFFSIZE);
	}

	namespace {
		constexpr uint32_t c_filter =
				FILE_NOTIFY_CHANGE_FILE_NAME |
				FILE_NOTIFY_CHANGE_CREATION |
				FILE_NOTIFY_CHANGE_SIZE |
				FILE_NOTIFY_CHANGE_DIR_NAME |
				FILE_NOTIFY_CHANGE_LAST_ACCESS |
				FILE_NOTIFY_CHANGE_LAST_WRITE;
	}
	void FNotify_depWin::_SetWaitNotify(Entry& ent) {
		ResetEvent(ent.hEvent.get());
		ent.ovl = OVERLAPPED{0};
		ent.ovl.hEvent = ent.hEvent.get();
		BOOL b = ReadDirectoryChangesW(ent.hDir.get(),
						ent.buff,
						FNotify_depWin::Entry::BUFFSIZE,
						TRUE,
						c_filter,
						nullptr,
						&ent.ovl,
						nullptr);
		Assert(Trap, b)
	}
	FNotify_depWin::FNotify_depWin() {
		_hEvent.reset(CreateEvent(nullptr, TRUE, FALSE, nullptr));
		_hThread.reset((HANDLE)_beginthreadex(nullptr, 0, ASyncLoop, this, 0, nullptr));
	}
	FNotify_depWin::~FNotify_depWin() {
		_csection.enter();
		_eventID = EventID::Exit;
		SetEvent(_hEvent.get());
		_csection.leave();
		WaitForSingleObject(_hThread.get(), INFINITE);
	}
	void FNotify_depWin::procEvent(CallbackD cb) {
		_csection.enter();
		for(auto& e : _eventL)
			cb(e);
		_eventL.clear();
		_csection.leave();
	}
	FNotify_depWin::DSC FNotify_depWin::addWatch(const std::string& path, uint32_t mask) {
		Entry& ent = _pathToEnt[path];
		ent.hDir.reset(CreateFile(path.c_str(),
						FILE_LIST_DIRECTORY,
						FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						nullptr,
						OPEN_EXISTING,
						FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
						nullptr));
		_dscToEntP[ent.hDir.get()] = &ent;
		ent.hEvent.reset(CreateEvent(nullptr, TRUE, FALSE, nullptr));
		ent.basePath = SPString(new std::string(path));

		_SetWaitNotify(ent);

		_csection.enter();
		_eventID = EventID::Add;
		SetEvent(_hEvent.get());
		_csection.leave();
		return ent.hDir.get();
	}
	void FNotify_depWin::remWatch(DSC dsc) {
		_csection.enter();
		auto itr = _dscToEntP.find(dsc);
		if(itr != _dscToEntP.end()) {
			_eventID = EventID::Rem;
			_eventArg = (*itr->second).hEvent.get();
			SetEvent(_hEvent.get());

			auto itr2 = _pathToEnt.find(*(*itr->second).basePath);
			_dscToEntP.erase(itr);
			_pathToEnt.erase(itr2);
		}
		_csection.leave();
	}
	void FNotify_depWin::_pushInfo(Entry& ent, FILE_NOTIFY_INFORMATION& pData, uint32_t cookie) {
		FileEvent id;
	    switch(pData.Action) {
		    case FILE_ACTION_ADDED:
				id = FE_Create;
				break;
		    case FILE_ACTION_REMOVED:
				id = FE_Remove;
				break;
		    case FILE_ACTION_MODIFIED:
				id = FE_Modify;
				break;
		    case FILE_ACTION_RENAMED_OLD_NAME:
				id = FE_MoveFrom;
				break;
		    case FILE_ACTION_RENAMED_NEW_NAME:
				id = FE_MoveTo;
				break;
		    default:
				Assert(Warn, false, "unknown action id")
	    }
		// (バイト数)
	    uint32_t lenBytes = pData.FileNameLength;
		Dir dir(*ent.basePath);
		c16Str name(reinterpret_cast<const char16_t*>(pData.FileName), lenBytes/sizeof(char16_t));
		dir <<= name;
		_eventL.push_back(Event{ent.hDir.get(), id, Text::UTFConvertTo8(name), cookie, dir.isDirectory()});
	}

	unsigned int __stdcall FNotify_depWin::ASyncLoop(void* ths_p) {
		FNotify_depWin* ths = reinterpret_cast<FNotify_depWin*>(ths_p);
		constexpr int MAX_HANDLES = 32;
		HANDLE handles[MAX_HANDLES];
		Entry* entries[MAX_HANDLES];
		for(;;) {
			ths->_csection.enter();
			int count = 0;
			for(auto& ent : ths->_pathToEnt) {
				handles[count] = ent.second.hEvent.get();
				entries[count] = &ent.second;
				++count;
			}
			Assert(Trap, count<=MAX_HANDLES)
			ths->_csection.leave();
			// リストの最後にメッセージ通知用イベントを置く
			handles[count++] = ths->_hEvent.get();

			DWORD res = WaitForMultipleObjects(count, handles, FALSE, INFINITE);
			ths->_csection.enter();
			Assert(Warn, res!=WAIT_FAILED, "failed to wait objects (in watch thread)")
			if(res >= WAIT_OBJECT_0 &&
			   res < WAIT_OBJECT_0+count-1)
			{
				res -= WAIT_OBJECT_0;
				Entry& ent = *entries[res];
				DWORD retsize;
				Assert(Trap, GetOverlappedResult(ent.hDir.get(), &ent.ovl, &retsize, FALSE))
				if(retsize == 0) {
					// バッファオーバーフロー
					Assert(Warn, false, "buffer overflowed")
				} else {
					// 1つずつイベントを読んでいく
					int ofs = 0;
					for(;;) {
						FILE_NOTIFY_INFORMATION* pData = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(ent.buff + ofs);
						ths->_pushInfo(ent, *pData, 0);
						if(pData->NextEntryOffset == 0)
							break;
						ofs += pData->NextEntryOffset;
					}
				}
				_SetWaitNotify(ent);
			} else {
				Assert(Trap, res == WAIT_OBJECT_0+count-1)
				if(ths->_eventID == EventID::Exit) {
					// 全てのIOをキャンセル
					for(auto& p : ths->_pathToEnt) {
						auto& ent = p.second;
						CancelIo(ent.hDir.get());
						WaitForSingleObject(ent.hEvent.get(), INFINITE);
					}
					break;
				}
				if(ths->_eventID == EventID::Rem) {
					// 動作中の非同期IOをキャンセル
					bool bFound = false;
					for(int i=0 ; i<count ; i++) {
						if(handles[i] == ths->_eventArg) {
							auto& ent = *entries[i];
							CancelIo(ent.hDir.get());
							WaitForSingleObject(ent.hEvent.get(), INFINITE);

							auto itr = ths->_dscToEntP.find(ent.hDir.get());
							ths->_dscToEntP.erase(itr);
							auto itr2 = std::find_if(ths->_pathToEnt.begin(), ths->_pathToEnt.end(), [&ent](PathToEnt::value_type& val) {
								return &val.second == &ent;
							});
							ths->_pathToEnt.erase(itr2);

							bFound = true;
							break;
						}
					}
					AssertP(Trap, bFound, "event handle not found in list")
				}
				ResetEvent(handles[res-WAIT_OBJECT_0]);
			}
			ths->_csection.leave();
		}
		_endthreadex(0);
		return 0;
	}
}
