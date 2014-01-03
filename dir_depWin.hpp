#pragma once
#include <intrin.h>
#include <windows.h>
#include "path.hpp"
#include <memory>

namespace spn {
	class WError : public std::runtime_error {
		public:
			WError(const WError&) = default;
			WError(const char* name);
	};

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
		static PathStr Getcwd();
		static void Chdir(ToPathStr path);
		static bool Chdir_nt(ToPathStr path);
		static void Mkdir(ToPathStr path, uint32_t mode);
		static void Chmod(ToPathStr path, uint32_t mode);
		static void Rmdir(ToPathStr path);
		static void Remove(ToPathStr path);
		static void Move(ToPathStr from, ToPathStr to);
		static void Copy(ToPathStr from, ToPathStr to);
		static void EnumEntry(ToPathStr path, EnumCBD cb);
		static FStatus Status(ToPathStr path);
		static bool IsFile(ToPathStr path);
		static bool IsDirectory(ToPathStr path);

		//! ファイルフラグ変換 (spinner -> WinAPI)
		static uint32_t ConvertFlag_S2W(uint32_t flag);
		//! ファイルフラグ変換 (WinAPI -> spinner)
		static uint32_t ConvertFlag_W2S(uint32_t flag);
		static PathStr GetCurrentDir();
		static void SetCurrentDir(const PathStr& path);
	};
	using DirDep = Dir_depWin;
}
