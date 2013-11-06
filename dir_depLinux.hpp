#pragma once
#include "path.hpp"

struct stat;
namespace spn {
	using ToPathStr = To8Str;
	using PathCh = char;
	using PathStr = std::basic_string<PathCh>;
	using EnumCBD = std::function<void (const PathCh*)>;

	struct Dir_depLinux {
		std::string getcwd() const;
		void chdir(To8Str path) const;
		bool chdir_nt(To8Str path) const;
		void mkdir(To8Str path, uint32_t mode) const;
		void chmod(To8Str path, uint32_t mode) const;
		void rmdir(To8Str path) const;
		void remove(To8Str path) const;
		void move(To8Str from, To8Str to) const;
		void copy(To8Str from, To8Str to) const;
		void enumEntry(To8Str path, EnumCBD cb) const;
		FStatus status(To8Str path) const;
		bool isFile(To8Str path) const;
		bool isDirectory(To8Str path) const;

		static FStatus CreateFStatus(const struct stat& st);
		//! ファイルフラグ変換 (spinner -> linux)
		static uint32_t ConvertFlag_S2L(uint32_t flag);
		//! ファイルフラグ変換 (linux -> spinner)
		static uint32_t ConvertFlag_L2S(uint32_t flag);
	};
	using DirDep = Dir_depLinux;
}
