#pragma once
#include "path.hpp"
#include <windows.h>
#include <memory>

namespace spn {
	FILETIME UnixTime2WinFT(time_t t);
	time_t WinFT2UnixTime(const FILETIME& ft);

	struct WinHandleDeleter {
		void operator()(HANDLE hdl) const {
			if(hdl != INVALID_HANDLE_VALUE)
				CloseHandle(hdl);
		}
	};
	//! WinAPIのHANDLEを管理するunique_ptr
	using UPWinH = std::unique_ptr<typename std::remove_pointer<HANDLE>::type, WinHandleDeleter>;

	// 本当はwchar_tだが中身はUTF-16なのでパス文字はchar16_tとして定義
	using ToPathStr = To16Str;
	using PathCh = char16_t;
	using PathStr = std::basic_string<PathCh>;
	using EnumCBD = std::function<void (const PathCh*, bool)>;

	struct Dir_depWin {
		PathStr getcwd() const;
		void chdir(ToPathStr path) const;
		bool chdir_nt(ToPathStr path) const;
		void mkdir(ToPathStr path, uint32_t mode) const;
		void chmod(ToPathStr path, uint32_t mode) const;
		void rmdir(ToPathStr path) const;
		void remove(ToPathStr path) const;
		void move(ToPathStr from, ToPathStr to) const;
		void copy(ToPathStr from, ToPathStr to) const;
		void enumEntry(ToPathStr path, EnumCBD cb) const;
		FStatus status(ToPathStr path) const;
		bool isFile(ToPathStr path) const;
		bool isDirectory(ToPathStr path) const;

		//! ファイルフラグ変換 (spinner -> WinAPI)
		static uint32_t ConvertFlag_S2W(uint32_t flag);
		//! ファイルフラグ変換 (WinAPI -> spinner)
		static uint32_t ConvertFlag_W2S(uint32_t flag);
	};
	using DirDep = Dir_depWin;
}
