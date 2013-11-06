#include "ziptree.hpp"
#include "path.hpp"
#include "dir.hpp"

namespace spn {
	namespace zip {
		FTree::FTree(spn::To8Str str): name(str.getStringPtr()) {}

		Directory::Directory(spn::To8Str str): FTree(str) {}
		void Directory::enumFile(const boost::regex& re, spn::PathBlock& lpath, EnumCB cb) const {
			if(!name.empty())
				lpath <<= name;
			for(auto& fl : flist)
				fl->enumFile(re, lpath, cb);
			if(!name.empty())
				lpath.popBack();
		}
		void Directory::addItem(int idx, spn::PathBlock& lpath) {
			if(lpath.segments() == 0)
				throw std::runtime_error("invalid path (addItem)");

			auto seg0 = lpath.getFirst_utf8();
			lpath.popFront();

			if(lpath.segments() == 0) {
				flist.push_back(UPtr<FTree>(new File(seg0, idx)));
			} else {
				auto itr = std::find_if(flist.begin(), flist.end(), [&seg0](const UPtr<FTree>& ft) {
					return ft->name == seg0;
				});
				if(itr == flist.end()) {
					flist.push_back(UPtr<FTree>(new Directory(seg0)));
					itr = flist.end()-1;
				}
				(*itr)->addItem(idx, lpath);
			}
		}
		const File* Directory::findFile(spn::PathBlock& lpath) const {
			auto seg0 = lpath.getFirst_utf8();
			lpath.popFront();
			for(auto& f : flist) {
				if(f->name == seg0)
					return f->findFile(lpath);
			}
			return nullptr;
		}

		File::File(spn::To8Str str, int idx): FTree(str), index(idx) {}
		void File::enumFile(const boost::regex& re, spn::PathBlock& lpath, EnumCB cb) const {
			lpath.pushBack(name);
			boost::smatch m;
			if(boost::regex_match(lpath.plain_utf8(), m, re))
				cb(lpath, *this);
			lpath.popBack();
		}
		void File::addItem(int idx, spn::PathBlock& lpath) {
			throw std::runtime_error("invalid operation (addItem)");
		}
		const File* File::findFile(spn::PathBlock& lpath) const {
			if(lpath.segments() > 0)
				throw std::runtime_error("invalid operation (findFile)");
			return this;
		}

		ZipTree::ZipTree(AdaptStream&& as): ZipTree(as) {}
		ZipTree::ZipTree(AdaptStream& as): ZipFile(std::move(as)), _froot("") {
			_init();
		}
		ZipTree::ZipTree(std::istream& ifs): ZipFile(AdaptStd(ifs)), _froot("") {
			_init();
		}
		void ZipTree::_init() {
			auto& hdr = headers();
			int nHdr = hdr.size();
			for(int i=0 ; i<nHdr ; i++) {
				auto* fp = hdr[i].get();
				spn::PathBlock lpath(fp->file_name);
				_froot.addItem(i, lpath);
			}
		}
		void ZipTree::enumFileWildCard(const std::string& wc, FTree::EnumCB cb) const {
			enumFileRegEx(boost::regex(Dir::ToRegEx(wc)), cb);
		}
		void ZipTree::enumFileRegEx(const boost::regex& re, FTree::EnumCB cb) const {
			spn::PathBlock pb;
			_froot.enumFile(re, pb, cb);
		}
		const File* ZipTree::findFile(spn::To32Str path) const {
			spn::PathBlock pb(path);
			return _froot.findFile(pb);
		}
	}
}
