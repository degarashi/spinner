#pragma once
#include <string>
#include <memory>
#include <map>
#include "macro.hpp"

namespace spn {
	using SPString = std::shared_ptr<std::string>;
	struct UserData {
		virtual ~UserData() {}
	};
	using SPData = std::shared_ptr<UserData>;

	// ファイル構造検知
	enum FileEvent {
		FE_Invalid = 0x00,
		FE_Create = 0x01,
		FE_Access = 0x02,
		FE_Modify = 0x04,
		FE_Remove = 0x08,
		FE_MoveFrom = 0x10,
		FE_MoveTo = 0x20,
		FE_Attribute = 0x40
	};
	struct FEv;
	using Callback = std::function<void (FEv&)>;

	template <class T>
	struct FNotifyEvent {
		T			dsc;
		FileEvent	event;
		std::string	path;
		uint32_t	cookie;
		bool		bDir;
		#define FNotifyEvent_Seq (dsc)(event)(path)(cookie)(bDir)

		FNotifyEvent() = default;
		FNotifyEvent(const FNotifyEvent&) = default;
		FNotifyEvent(FNotifyEvent&& e): Move_Ctor(FNotifyEvent_Seq, e) {}
		template <class T2, class T3>
		FNotifyEvent(T2&& ds, const FileEvent& ev, T3&& p, uint32_t c, bool dir):
			dsc(std::forward<T2>(ds)), event(ev), path(std::forward<T3>(p)), cookie(c), bDir(dir) {}
	};
}
