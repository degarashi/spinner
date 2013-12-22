#include "dir_depWin.hpp"
#include "error.hpp"
#include "optional.hpp"

namespace {
	class WError : public std::runtime_error {
		public:
			WError(const WError&) = default;
			WError(const char* name): std::runtime_error("") {
				LPVOID lpMsg;
				FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
							nullptr,
							GetLastError(),
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							reinterpret_cast<LPWSTR>(&lpMsg),
							0, nullptr);
				spn::PathStr pstr(reinterpret_cast<const char16_t*>(lpMsg));
				LocalFree(lpMsg);

				std::stringstream ss;
				ss << "winapi-error at " << name << std::endl;
				std::string msg = spn::Text::UTFConvertTo8(pstr);
				ss.write(msg.c_str(), msg.length());
				static_cast<std::runtime_error&>(*this) = std::runtime_error(ss.str());
			}
	};
}
#define AS_LPCWSTR_PATH(path)	reinterpret_cast<const wchar_t*>(path.getStringPtr())
#define AS_LPWSTR(path)			reinterpret_cast<wchar_t*>(path)
//MEMO: ファイルのユーザーアクセス権限関係はとりあえず無視
namespace spn {
	PathStr Dir_depWin::getcwd() const {
		PathCh tmp[512];
		GetCurrentDirectoryW(sizeof(tmp)/sizeof(tmp[0]), AS_LPWSTR(tmp));
		return PathStr(tmp);
	}
	void Dir_depWin::chdir(ToPathStr path) const {
		AssertT(Throw, SetCurrentDirectoryW(AS_LPCWSTR_PATH(path)),
				(WError)(const char*), "chdir")
	}
	bool Dir_depWin::chdir_nt(ToPathStr path) const {
		return SetCurrentDirectoryW(AS_LPCWSTR_PATH(path)) != FALSE;
	}
	void Dir_depWin::mkdir(ToPathStr path, uint32_t mode) const {
		AssertT(Throw, CreateDirectoryW(AS_LPCWSTR_PATH(path), nullptr) == 0,
				(WError)(const char*), "mkdir")
	}
	void Dir_depWin::chmod(ToPathStr path, uint32_t mode) const {
		// Windowsだとファイルにデフォルトで実行権限がついてるので何もしない
	}
	void Dir_depWin::rmdir(ToPathStr path) const {
		AssertT(Throw, RemoveDirectoryW(AS_LPCWSTR_PATH(path)) != 0,
				(WError)(const char*), "rmdir")
	}
	void Dir_depWin::remove(ToPathStr path) const {
		AssertT(Throw, DeleteFileW(AS_LPCWSTR_PATH(path)) != 0,
				(WError)(const char*), "remove")
	}
	void Dir_depWin::move(ToPathStr from, ToPathStr to) const {
		AssertT(Throw, MoveFileW(AS_LPCWSTR_PATH(from), AS_LPCWSTR_PATH(to)) != 0,
				(WError)(const char*), "move")
	}
	void Dir_depWin::copy(ToPathStr from, ToPathStr to) const {
		AssertT(Throw, CopyFileW(AS_LPCWSTR_PATH(from), AS_LPCWSTR_PATH(to), TRUE) != 0,
				(WError)(const char*), "copy")
	}
	void Dir_depWin::enumEntry(ToPathStr path, EnumCBD cb) const {
		WIN32_FIND_DATAW tfd;
		UPWinH fh(FindFirstFileW(AS_LPCWSTR_PATH(path), &tfd));
		if(fh.get() == INVALID_HANDLE_VALUE)
			return;
		do {
			cb(reinterpret_cast<const char16_t*>(tfd.cFileName), tfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		} while(FindNextFileW(fh.get(), &tfd));
	}
	FStatus Dir_depWin::status(ToPathStr path) const {
		uint32_t attr = GetFileAttributesW(AS_LPCWSTR_PATH(path));
		if(attr == INVALID_FILE_ATTRIBUTES)
			return FStatus(FStatus::NotAvailable);

		FStatus st;
		// Windowsの場合はユーザーIDやグループIDが無効 (=0)
		st.userId = 0;
		st.groupId = 0;
		UPWinH fh(CreateFileW(AS_LPCWSTR_PATH(path), GENERIC_READ, FILE_SHARE_READ,
								nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
		LARGE_INTEGER sz;
		GetFileSizeEx(fh.get(), &sz);
		st.size = sz.QuadPart;
		st.flag = ConvertFlag_W2S(attr);
		FILETIME tmC, tmA, tmW;
		GetFileTime(fh.get(), &tmC, &tmA, &tmW);

		st.tmCreated = WinTime2UnixTime(tmC);
		st.tmAccess = WinTime2UnixTime(tmA);
		st.tmModify = WinTime2UnixTime(tmW);
		return st;
	}
	namespace {
		constexpr int UnixEpocYear = 1970,
					WinEpocYear = 1601;
		constexpr uint64_t Second = uint64_t(100 * 1000000),
							Minute = Second * 60,
							Hour = Minute * 60,
							Day = Hour * 24,
							Year = Day * 365;
	}
	FILETIME UnixTime2WinTime(time_t t) {
		uint64_t tmp = t * Second;
		tmp -= (UnixEpocYear - WinEpocYear) * Year;
		FILETIME ft;
		ft.dwLowDateTime = tmp & 0xffffffff;
		tmp >>= 32;
		ft.dwHighDateTime = tmp;
		return ft;
	}
	time_t WinTime2UnixTime(FILETIME ft) {
		uint64_t tmp = ft.dwHighDateTime;
		tmp <<= 32;
		tmp |= ft.dwLowDateTime;
		tmp += (UnixEpocYear - WinEpocYear) * Year;
		tmp /= Second;
		return static_cast<time_t>(tmp);
	}

	uint32_t Dir_depWin::ConvertFlag_S2W(uint32_t flag) {
		uint32_t res = 0;
		if(flag & FStatus::DirectoryType)
			res |= FILE_ATTRIBUTE_DIRECTORY;
		constexpr uint32_t AllRWE = (FStatus::AllExec|FStatus::AllWrite);
		if((flag & AllRWE) != AllRWE)
			res |= FILE_ATTRIBUTE_READONLY;
		else
			res = FILE_ATTRIBUTE_NORMAL;
		return res;
	}
	uint32_t Dir_depWin::ConvertFlag_W2S(uint32_t flag) {
		// ユーザー書き込み権限を持っていなければReadOnlyとする
		uint32_t res = 0;
		if(flag == FILE_ATTRIBUTE_NORMAL)
			return FStatus::FileType;
		res |= (flag & (FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_READONLY)) ? FStatus::AllRead : (FStatus::AllRW|FStatus::AllExec);
		res |= (flag & FILE_ATTRIBUTE_DIRECTORY) ? FStatus::DirectoryType : FStatus::FileType;
		return res;
	}
	namespace {
		spn::Optional<bool> SingleCheck(const ToPathStr& path, uint32_t flag) {
			uint32_t attr = GetFileAttributesW(AS_LPCWSTR_PATH(path));
			if(attr == INVALID_FILE_ATTRIBUTES)
				return spn::none;
			return attr & flag;
		}
	}
	bool Dir_depWin::isFile(ToPathStr path) const {
		auto res = SingleCheck(path, FILE_ATTRIBUTE_DIRECTORY);
		if(!res)
			return false;
		return !(*res);
	}
	bool Dir_depWin::isDirectory(ToPathStr path) const {
		auto res = SingleCheck(path, FILE_ATTRIBUTE_DIRECTORY);
		return res && *res;
	}
}

