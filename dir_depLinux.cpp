#include "dir_depLinux.hpp"
#ifdef ANDROID
	#include <dirent.h>
	#include <errno.h>
#else
	#include <sys/dir.h>
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include <sstream>
#include <memory>
#include <fstream>
#include <iostream>
#include "error.hpp"

namespace {
	class PError : public std::runtime_error {
		public:
			PError(const PError&) = default;
			PError(const char* name): std::runtime_error("") {
				perror(name);
				std::stringstream ss;
				ss << "posix-error at " << name << ' ' << errno << std::endl;
				(std::runtime_error&)(*this) = std::runtime_error(ss.str());
			}
	};
}
namespace spn {
	std::string Dir_depLinux::getcwd() const {
		char tmp[512];
		::getcwd(tmp, sizeof(tmp));
		return std::string(tmp);
	}
	void Dir_depLinux::chdir(To8Str path) const {
		AssertT(Throw, ::chdir(path.getPtr()) >= 0, (PError)(const char*), "chdir")
	}
	bool Dir_depLinux::chdir_nt(To8Str path) const {
		return ::chdir(path.getPtr()) >= 0;
	}
	void Dir_depLinux::mkdir(To8Str path, uint32_t mode) const {
		AssertT(Throw, ::mkdir(path.getPtr(), ConvertFlag_S2L(mode)) >= 0, (PError)(const char*), "mkdir")
	}
	void Dir_depLinux::chmod(To8Str path, uint32_t mode) const {
		AssertT(Throw, ::chmod(path.getPtr(), ConvertFlag_S2L(mode)) >= 0, (PError)(const char*), "chmod")
	}
	void Dir_depLinux::rmdir(To8Str path) const {
		AssertT(Throw, ::rmdir(path.getPtr()) >= 0, (PError)(const char*), "rmdir")
	}
	void Dir_depLinux::remove(To8Str path) const {
		AssertT(Throw, ::unlink(path.getPtr()) >= 0, (PError)(const char*), "remove")
	}
	void Dir_depLinux::move(To8Str from, To8Str to) const {
		AssertT(Throw, ::rename(from.getPtr(), to.getPtr()) >= 0, (PError)(const char*), "move")
	}
	void Dir_depLinux::copy(To8Str from, To8Str to) const {
		using Size = std::streamsize;
		std::ifstream ifs(from.getPtr(), std::ios::binary);
		std::ofstream ofs(to.getPtr(), std::ios::binary|std::ios::trunc);
		ifs.seekg(std::ios::end);
		Size sz = ifs.tellg();
		ifs.seekg(std::ios::beg);

		char buff[1024];
		for(;;) {
			Size nt = std::min(Size(sizeof(buff)), sz);
			ifs.read(buff, nt);
			ofs.write(buff, nt);
			if(nt != Size(sizeof(buff)))
				break;
		}
	}

	void Dir_depLinux::enumEntry(To8Str path, EnumCBD cb) const {
		struct DIR_Rel {
			void operator()(DIR* dir) const {
				closedir(dir);
			}
		};
		std::unique_ptr<DIR, DIR_Rel> dir(opendir(path.getPtr()));
		if(dir) {
			while(dirent* ent = readdir(dir.get()))
				cb(ent->d_name, S_ISDIR(ent->d_type));
		}
	}
	FStatus Dir_depLinux::status(To8Str path) const {
		struct stat st;
		if(stat(path.getPtr(), &st) < 0)
			return FStatus(FStatus::NotAvailable);
		return CreateFStatus(st);
	}

	namespace {
		const std::pair<FStatus::Flag, uint32_t> c_f2s[] = {
			{FStatus::UserRead, S_IRUSR},
			{FStatus::UserWrite, S_IWUSR},
			{FStatus::UserExec, S_IXUSR},
			{FStatus::GroupRead, S_IRGRP},
			{FStatus::GroupWrite, S_IWGRP},
			{FStatus::GroupExec, S_IXGRP},
			{FStatus::OtherRead, S_IROTH},
			{FStatus::OtherWrite, S_IWOTH},
			{FStatus::OtherExec, S_IXOTH}
		};
	}
	FStatus Dir_depLinux::CreateFStatus(const struct stat& st) {
		FStatus fs;
		fs.flag = ConvertFlag_L2S(st.st_mode);
		fs.userId = st.st_uid;
		fs.groupId = st.st_gid;
		fs.size = st.st_size;
		fs.tmAccess = st.st_atime;
		fs.tmModify = st.st_mtime;
		fs.tmCreated = st.st_ctime;
		return fs;
	}
	uint32_t Dir_depLinux::ConvertFlag_L2S(uint32_t flag) {
		uint32_t res = 0;
		if(S_ISREG(flag))
			res |= FStatus::FileType;
		else if(S_ISDIR(flag))
			res |= FStatus::DirectoryType;

		for(auto& p : c_f2s) {
			if(p.second & flag)
				res |= p.first;
		}
		return res;
	}
	uint32_t Dir_depLinux::ConvertFlag_S2L(uint32_t flag) {
		uint32_t res = 0;
		if(flag & FStatus::FileType)
			res |= S_IFREG;
		else if(flag & FStatus::DirectoryType)
			res |= S_IFDIR;

		for(auto& p : c_f2s) {
			if(p.first & flag)
				res |= p.second;
		}
		return res;
	}
	bool Dir_depLinux::isFile(To8Str path) const {
		struct stat st;
		return stat(path.getPtr(), &st) >= 0 && S_ISREG(st.st_mode);
	}
	bool Dir_depLinux::isDirectory(To8Str path) const {
		struct stat st;
		return stat(path.getPtr(), &st) >= 0 && S_ISDIR(st.st_mode);
	}
}
