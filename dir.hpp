#pragma once
#if defined(UNIX)
	#include "dir_depLinux.hpp"
#elif defined(WIN32)
	#include "dir_depWin.hpp"
#else
	#error "unknown OS"
#endif
#include <boost/regex.hpp>

namespace spn {
	//! ディレクトリ管理
	class Dir : public PathBlock {
		using ToStrType = typename DirDep::ToStrType;
		using ChType = typename DirDep::ChType;
		using StrType = std::basic_string<ChType>;
		struct PathReset {
			const DirDep& dep;
			std::string cwd;

			PathReset(const DirDep& d): dep(d), cwd(dep.getcwd()) {}
			~PathReset() { dep.chdir(cwd); }
		};

		using StrList = std::vector<std::string>;
		using EnumCB = std::function<void (std::string&&)>;
		using ModCB = std::function<bool (const PathBlock&, FStatus&)>;
		DirDep	_dep;
		void _enumEntryRegExR(const boost::regex& r, std::string& lpath, size_t baseLen, EnumCB cb) const;
		void _enumEntryRegEx(const boost::regex& r, const std::string& path, EnumCB cb) const;
		void _chmod(PathBlock& lpath, ModCB cb);
		public:
			using PathBlock::PathBlock;
			Dir(Dir&& d);

			FStatus status() const;
			//! パスまでのディレクトリ構造を作成
			/*! \return true=成功(既に同じ構造ができているケース含)
			*			false=失敗(ファイルが存在するなど) */
			void mkdir(uint32_t mode) const;
			//! ファイル/ディレクトリ列挙
			/*! \param[in] path 検索パス(正規表現) */
			StrList enumEntryRegEx(const boost::regex& r, bool bRecursive) const;
			/*! \param[in] path 検索パス(ワイルドカード) */
			StrList enumEntryWildCard(const std::string& s, bool bRecursive) const;
			void enumEntryRegEx(const boost::regex& r, EnumCB cb, bool bRecursive) const;
			void enumEntryWildCard(const std::string& s, EnumCB cb, bool bRecursives) const;
			void chmod(ModCB cb);
			void chmod(uint32_t mode);
			bool ChMod(const PathBlock& pb, ModCB cb);
			FILE* openAsFP(const char* mode) const;

			//! ワイルドカードから正規表現への書き換え
			static boost::regex ToRegEx(const std::string& s);
	};
}
