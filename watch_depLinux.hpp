#pragma once
#include "watch_common.hpp"

namespace spn {
	struct FNotify_depLinux {
		int		fd;
		using DSC = int;
		//! Dependant-Desc, FileEvent, FileName, CookieID, IsDir
		using CallbackD = std::function<void (const DSC&, FileEvent, const char*, uint32_t, bool)>;

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
