#pragma once
#include "zip.hpp"
#include "misc.hpp"
#include "path.hpp"
#include <boost/regex.hpp>

namespace spn {
	namespace zip {
		struct File;
		struct FTree {
			std::string name;

			FTree(spn::To8Str str);
			using EnumCB = std::function<void (const spn::PathBlock&, const File&)>;
			virtual void addItem(const DirHeader* hdr, spn::PathBlock& lpath) = 0;
			virtual void enumFile(const boost::regex& re, spn::PathBlock& lpath, EnumCB cb) const = 0;
			virtual const File* findFile(spn::PathBlock& lpath) const = 0;
		};
		struct Directory : FTree {
			using FList = std::vector<UPtr<FTree>>;
			FList		flist;

			Directory(spn::To8Str str);
			void addItem(const DirHeader* hdr, spn::PathBlock& lpath) override;
			void enumFile(const boost::regex& re, spn::PathBlock& lpath, EnumCB cb) const override;
			const File* findFile(spn::PathBlock& lpath) const override;
		};
		struct File : FTree {
			using Pos = std::istream::streampos;
			const DirHeader* header;

			File(spn::To8Str str, const DirHeader* hdr);
			void addItem(const DirHeader* hdr, spn::PathBlock& lpath) override;
			void enumFile(const boost::regex& re, spn::PathBlock& lpath, EnumCB cb) const override;
			const File* findFile(spn::PathBlock& lpath) const override;
		};
		//! Zipファイルを解析してツリー構造にする
		class ZipTree {
			ZipFile		_zf;
			Directory	_froot;

			FTree* _buildTree(Directory* pdir, spn::PathBlock& lpath);
			public:
				ZipTree(std::istream& ifs);
				void enumFile(const boost::regex& re, FTree::EnumCB cb) const;
				const File* findFile(spn::To32Str path) const;
		};
	}
}

