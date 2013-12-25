#pragma once
#include "path.hpp"

struct stat;
namespace spn {
	using ToPathStr = To8Str;
	using PathCh = char;
	using PathStr = std::basic_string<PathCh>;
	using EnumCBD = std::function<void (const PathCh*, bool)>;

	struct Dir_depLinux {
		std::string getcwd() const;
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
		FTime filetime(ToPathStr path) const;
		bool isFile(ToPathStr path) const;
		bool isDirectory(ToPathStr path) const;

		static FStatus CreateFStatus(const struct stat& st);
		//! ファイルフラグ変換 (spinner -> linux)
		static uint32_t ConvertFlag_S2L(uint32_t flag);
		//! ファイルフラグ変換 (linux -> spinner)
		static uint32_t ConvertFlag_L2S(uint32_t flag);
	};
	using DirDep = Dir_depLinux;
}
