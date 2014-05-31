#pragma once
#include "zip.hpp"
#include "misc.hpp"
#include "path.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#include <boost/regex.hpp>
#pragma GCC diagnostic pop

namespace spn {
	namespace zip {
		struct File;
		struct FTree {
			std::string name;

			FTree(spn::To8Str str);
			using EnumCB = std::function<void (const spn::PathBlock&, const File&)>;
			virtual void addItem(int idx, spn::PathBlock& lpath) = 0;
			virtual void enumFile(const boost::regex& re, spn::PathBlock& lpath, EnumCB cb) const = 0;
			virtual const File* findFile(spn::PathBlock& lpath) const = 0;
		};
		struct Directory : FTree {
			using FList = std::vector<UPtr<FTree>>;
			FList		flist;

			Directory(spn::To8Str str);
			void addItem(int idx, spn::PathBlock& lpath) override;
			void enumFile(const boost::regex& re, spn::PathBlock& lpath, EnumCB cb) const override;
			const File* findFile(spn::PathBlock& lpath) const override;
		};
		struct File : FTree {
			using Pos = AdaptStream::streampos;
			int			index;

			File(spn::To8Str str, int idx);
			void addItem(int idx, spn::PathBlock& lpath) override;
			void enumFile(const boost::regex& re, spn::PathBlock& lpath, EnumCB cb) const override;
			const File* findFile(spn::PathBlock& lpath) const override;
		};
		//! Zipファイルを解析してツリー構造にする
		class ZipTree : public ZipFile {
			Directory	_froot;

			FTree* _buildTree(Directory* pdir, spn::PathBlock& lpath);
			void _init();
			public:
				ZipTree(std::istream& ifs);
				ZipTree(AdaptStream& as);
				ZipTree(AdaptStream&& as);
				void enumFileWildCard(const std::string& wc, FTree::EnumCB cb) const;
				void enumFileRegEx(const boost::regex& re, FTree::EnumCB cb) const;
				const File* findFile(spn::To32Str path) const;
		};
	}
}

