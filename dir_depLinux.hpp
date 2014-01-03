#pragma once
#include "path.hpp"

#ifdef Status
	#undef Status
#endif

struct stat;
namespace spn {
	using ToPathStr = To8Str;
	using PathCh = char;
	using PathStr = std::basic_string<PathCh>;
	using EnumCBD = std::function<void (const PathCh*, bool)>;

	struct Dir_depLinux {
		static std::string Getcwd();
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
		static FTime Filetime(ToPathStr path);
		static bool IsFile(ToPathStr path);
		static bool IsDirectory(ToPathStr path);

		static FStatus CreateFStatus(const struct stat& st);
		//! ファイルフラグ変換 (spinner -> linux)
		static uint32_t ConvertFlag_S2L(uint32_t flag);
		//! ファイルフラグ変換 (linux -> spinner)
		static uint32_t ConvertFlag_L2S(uint32_t flag);
		static PathStr GetCurrentDir();
		static void SetCurrentDir(const PathStr& path);
	};
	using DirDep = Dir_depLinux;
}
