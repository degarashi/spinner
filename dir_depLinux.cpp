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
			PError(const std::string& name): std::runtime_error("") {
				perror(name.c_str());
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
	void Dir_depLinux::chdir(ToPathStr path) const {
		AssertT(Throw, ::chdir(path.getStringPtr()) >= 0, (PError)(const std::string&),
				std::string("chdir: ") + path.getStringPtr())
	}
	bool Dir_depLinux::chdir_nt(ToPathStr path) const {
		return ::chdir(path.getStringPtr()) >= 0;
	}
	void Dir_depLinux::mkdir(ToPathStr path, uint32_t mode) const {
		AssertT(Throw, ::mkdir(path.getStringPtr(), ConvertFlag_S2L(mode)) >= 0, (PError)(const std::string&),
				std::string("mkdir: ") + path.getStringPtr())
	}
	void Dir_depLinux::chmod(ToPathStr path, uint32_t mode) const {
		AssertT(Throw, ::chmod(path.getStringPtr(), ConvertFlag_S2L(mode)) >= 0, (PError)(const std::string&),
				std::string("chmod: ") + path.getStringPtr())
	}
	void Dir_depLinux::rmdir(ToPathStr path) const {
		AssertT(Throw, ::rmdir(path.getStringPtr()) >= 0, (PError)(const std::string&),
				std::string("rmdir: ") + path.getStringPtr())
	}
	void Dir_depLinux::remove(ToPathStr path) const {
		AssertT(Throw, ::unlink(path.getStringPtr()) >= 0, (PError)(const std::string&),
				std::string("remove: ") + path.getStringPtr())
	}
	void Dir_depLinux::move(ToPathStr from, ToPathStr to) const {
		AssertT(Throw, ::rename(from.getStringPtr(), to.getStringPtr()) >= 0, (PError)(const std::string&),
				std::string("move: ") + from.getStringPtr() + " -> " + to.getStringPtr())
	}
	void Dir_depLinux::copy(ToPathStr from, ToPathStr to) const {
		using Size = std::streamsize;
		std::ifstream ifs(from.getStringPtr(), std::ios::binary);
		std::ofstream ofs(to.getStringPtr(), std::ios::binary|std::ios::trunc);
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

	void Dir_depLinux::enumEntry(ToPathStr path, EnumCBD cb) const {
		struct DIR_Rel {
			void operator()(DIR* dir) const {
				closedir(dir);
			}
		};
		std::unique_ptr<DIR, DIR_Rel> dir(opendir(path.getStringPtr()));
		if(dir) {
			while(dirent* ent = readdir(dir.get()))
				cb(ent->d_name, S_ISDIR(ent->d_type));
		}
	}
	FStatus Dir_depLinux::status(ToPathStr path) const {
		struct stat st;
		if(::stat(path.getStringPtr(), &st) < 0)
			return FStatus(FStatus::NotAvailable);
		return CreateFStatus(st);
	}
	FTime Dir_depLinux::filetime(ToPathStr path) const {
		struct stat st;
		AssertT(Throw, ::stat(path.getStringPtr(), &st) >= 0, (PError)(const std::string&),
				std::string("filetime: ") + path.getStringPtr())
		FTime ft;
		ft.tmAccess = UnixTime2WinTime(st.st_atime);
		ft.tmCreated = UnixTime2WinTime(st.st_ctime);
		ft.tmModify = UnixTime2WinTime(st.st_mtime);
		return ft;
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
		fs.ftime.tmAccess = UnixTime2WinTime(st.st_atime);
		fs.ftime.tmModify = UnixTime2WinTime(st.st_mtime);
		fs.ftime.tmCreated = UnixTime2WinTime(st.st_ctime);
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
	bool Dir_depLinux::isFile(ToPathStr path) const {
		struct stat st;
		return stat(path.getStringPtr(), &st) >= 0 && S_ISREG(st.st_mode);
	}
	bool Dir_depLinux::isDirectory(ToPathStr path) const {
		struct stat st;
		return stat(path.getStringPtr(), &st) >= 0 && S_ISDIR(st.st_mode);
	}
}
