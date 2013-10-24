#pragma once
#include DIR_DEPEND_HEADER
#include <boost/regex.hpp>

namespace spn {
	//! ディレクトリ管理
	class Dir : public PathBlock {
		struct PathReset {
			const DirDep& dep;
			std::string cwd;

			PathReset(const DirDep& d): dep(d), cwd(dep.getcwd()) {}
			~PathReset() { dep.chdir(cwd); }
		};

		using StrList = std::vector<std::string>;
		using EnumCB = std::function<void (std::string&&)>;
		DirDep	_dep;
		void _enumEntryRegExR(const boost::regex& r, std::string& lpath, size_t baseLen, EnumCB cb) const;
		void _enumEntryRegEx(const boost::regex& r, const std::string& path, EnumCB cb) const;
		void _chmod(std::string& lpath, uint32_t mode);
		public:
			using PathBlock::PathBlock;
			Dir(Dir&& d);

			FStatus status() const;
			//! パスまでのディレクトリ構造を作成
			/*! \return true=成功(既に同じ構造ができているケース含)
			*					false=失敗(ファイルが存在するなど) */
			void mkdir(uint32_t mode) const;
			File asFile() const;
			//! ファイル/ディレクトリ列挙
			/*! \param[in] path 検索パス(正規表現) */
			StrList enumEntryRegEx(const boost::regex& r, bool bRecursive) const;
			/*! \param[in] path 検索パス(ワイルドカード) */
			StrList enumEntryWildCard(const std::string& s, bool bRecursive) const;
			void enumEntryRegEx(const boost::regex& r, EnumCB cb, bool bRecursive) const;
			void enumEntryWildCard(const std::string& s, EnumCB cb, bool bRecursives) const;
			void chmod(uint32_t mode, bool bRecursive=false);

			//! ワイルドカードから正規表現への書き換え
			static boost::regex ToRegEx(const std::string& s);
	};
}
