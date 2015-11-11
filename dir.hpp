#pragma once
#if defined(UNIX)
	#include "dir_depLinux.hpp"
#elif defined(WIN32)
	#include "dir_depWin.hpp"
#else
	#error "unknown OS"
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#include <boost/regex.hpp>
#pragma GCC diagnostic pop

namespace spn {
	//! ディレクトリ管理
	class Dir : public PathBlock {
		public:
			struct PathReset {
				PathStr 		cwd;

				PathReset(): cwd(DirDep::Getcwd()) {}
				~PathReset() { DirDep::Chdir(cwd); }
			};
		private:
			const static char SC, DOT, EOS, *SC_P, LBK, RBK;

			using RegexL = std::vector<boost::regex>;
			using RegexItr = RegexL::const_iterator;
			using StrList = std::vector<std::string>;
			using EnumCB = std::function<void (const Dir&)>;
			using ModCB = std::function<bool (const PathBlock&, FStatus&)>;
			//! '/'で区切ったRegExの文法をリスト形式に直す
			static RegexL _ParseRegEx(const std::string& r);

			static void _EnumEntryRegEx(RegexItr itr, RegexItr itrE, std::string& lpath, size_t baseLen, EnumCB cb);
			static void _EnumEntry(const std::string& s, const std::string& path, EnumCB cb);
			void _chmod(PathBlock& lpath, ModCB cb);
		public:
			using PathBlock::PathBlock;
			Dir() = default;
			Dir(const Dir&) = default;
			Dir(Dir&& d);

			Dir& operator = (Dir&& d);
			Dir& operator = (const Dir&) = default;
			using PathBlock::operator ==;

			bool isFile() const;
			bool isDirectory() const;
			void remove() const;
			void copy(const std::string& to) const;
			void move(const std::string& to) const;
			FStatus status() const;
			FTime getTime() const;
			//! パスまでのディレクトリ構造を作成
			/*! \return true=成功(既に同じ構造ができているケース含)
			*			false=失敗(ファイルが存在するなど) */
			void mkdir(uint32_t mode) const;
			//! ファイル/ディレクトリ列挙
			/*! \param[in] path 検索パス(正規表現) 区切り文字を跨いでのマッチは不可<br>
								相対パスならDirが指しているディレクトリ以下の階層を探索 */
			static StrList EnumEntryRegEx(const std::string& r);
			/*! \param[in] path 検索パス(ワイルドカード) 区切り文字を跨いでのマッチは不可 */
			static StrList EnumEntryWildCard(const std::string& s);
			static void EnumEntryRegEx(const std::string& r, EnumCB cb);
			static void EnumEntryWildCard(const std::string& s, EnumCB cb);
			void chmod(ModCB cb);
			void chmod(uint32_t mode);
			bool ChMod(const PathBlock& pb, ModCB cb);
			FILE* openAsFP(const char* mode) const;
			/*! \return 前のカレントパス */
			std::string setCurrentDir() const;

			//! ワイルドカードから正規表現への書き換え
			static std::string ToRegEx(const std::string& s);
			static std::string GetCurrentDir();
			static std::string GetProgramDir();
			static void SetCurrentDir(const std::string& path);
			std::string plain_utf8(bool bAbs=true) const;
			std::u32string plain_utf32(bool bAbs=true) const;
	};
}
