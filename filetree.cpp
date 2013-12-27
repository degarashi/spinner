#include "filetree.hpp"
#include "dir.hpp"
#include "watch.hpp"

namespace spn {
	namespace {
		void InsertIndent(std::ostream& os, int n) {
			for(int i=0 ; i<n ; i++)
				os << '\t';
		}
	}
	// ------------------- FileInfo -------------------
	FileTree::Type FileInfo::getType() const { return FileTree::Type::File; }
	bool FileInfo::isDirectory() const { return false; }
	void FileInfo::print(std::ostream& os, int indent) const {
		InsertIndent(os, indent);
		os << "File: " << fname << std::endl;
	}

	// ------------------- DirInfo -------------------
	FileTree::Type DirInfo::getType() const { return FileTree::Type::Directory; }
	bool DirInfo::isDirectory() const { return true; }
	bool DirInfo::operator == (const IFTFile& f) const {
		if(static_cast<const IFTFile&>(*this) == f) {
			auto& fd = reinterpret_cast<const DirInfo&>(f);
			if(fmap.size() == fd.fmap.size()) {
				for(auto& e : fmap) {
					auto itr = fd.fmap.find(e.first);
					if(itr == fd.fmap.end())
						return false;
					if(*e.second != *itr->second)
						return false;
				}
			}
		}
		return false;
	}
	void DirInfo::print(std::ostream& os, int indent) const {
		InsertIndent(os, indent);
		os << "Dir: " << fname << std::endl;
		for(auto& p : fmap)
			p.second->print(os, indent+1);
	}

	#define AS_LPCWSTR(path)	reinterpret_cast<const wchar_t*>(To16Str(path).getStringPtr())
	#define AS_UTF8(path)		To8Str(reinterpret_cast<const char16_t*>(path)).getStringPtr()
	// ------------------- FileTree -------------------
	//! 続きから走査
	UPIFile FileTree::ConstructFTree(Dir& dir) {
		DirInfo* pd;
		UPIFile upd(pd = new DirInfo(dir.getLast_utf8(), dir.status()));

		// 下の階層を読む
		dir.enumEntryWildCard("*", [pd, &dir](const Dir& d) {
			auto fs = d.status();
			auto last = d.getLast_utf32();
			dir <<= last;
			auto last8 = Text::UTFConvertTo8(last);
			pd->fmap.emplace(last8.c_str(),
							 (fs.flag & FStatus::DirectoryType) ?
							 ConstructFTree(dir) :
							 UPIFile(new FileInfo(last8, fs)));
			dir.popBack();
		});
		return std::move(upd);
	}

	//! ツリートップから走査
	UPIFile FileTree::ConstructFTree(To32Str path) {
		Dir dir(path);
		Assert(Trap, dir.status().flag & FStatus::DirectoryType, "path is not directory")
		return ConstructFTree(dir);
	}

	void FileTree::For_Each(const IFTFile* ft, std::function<void(const std::string&, const IFTFile*)> cb, PathBlock& pblock) {
		AssertP(Trap, ft->isDirectory())
		auto* d = reinterpret_cast<const DirInfo*>(ft);
		for(auto& e : d->fmap) {
			auto* ent = e.second.get();
			pblock <<= e.first;
			cb(pblock.plain_utf8(), ent);
			if(ent->isDirectory())
				For_Each(ent, cb, pblock);
			pblock.popBack();
		}
	}
	void FileTree::ThrowCreateEvent(FRecvNotify& visitor, const SPString& base, const IFTFile* ft, PathBlock& pblock) {
		For_Each(ft, [&](const std::string& str, const IFTFile* ft) {
			visitor.event(FEv_Create(base, str, ft->isDirectory()), SPData());
		}, pblock);
	}
	void FileTree::ThrowRemoveEvent(FRecvNotify& visitor, const SPString& base, const IFTFile* ft, PathBlock& pblock) {
		For_Each(ft, [&](const std::string& str, const IFTFile* ft) {
			visitor.event(FEv_Remove(base, str, ft->isDirectory()), SPData());
		}, pblock);
	}
	void FileTree::VisitDifference(FRecvNotify& visitor, const SPString& base, const IFTFile* f0, const IFTFile* f1, PathBlock& pblock) {
		AssertP(Trap, f0->isDirectory() && f1->isDirectory())
		auto *fd0 = reinterpret_cast<const DirInfo*>(f0),
			*fd1 = reinterpret_cast<const DirInfo*>(f1);
		for(auto& ent : fd0->fmap) {
			auto itr = fd1->fmap.find(ent.first);
			pblock <<= ent.first;
			// 同名のエントリがFile -> Directory等と種別が変わっていたらRemove, Createと解釈
			bool bDir = ent.second->isDirectory();
			if(itr == fd1->fmap.end()) {
				// エントリが削除された
				visitor.event(FEv_Remove(base, pblock.plain_utf8(), bDir), SPData());
				// Directoryならば下層のエントリが全部イベント呼び出し対象 -> FEv_Remove
				if(bDir)
					ThrowRemoveEvent(visitor, base, ent.second.get(), pblock);
			} else {
				FStatus fs0 = ent.second->getStatus();
				const FStatus& fs1 = itr->second->getStatus();
				std::string path8(pblock.plain_utf8());
				// アクセス時刻チェック => FEv_Access
				// ディレクトリのアクセスチェックはしない
				if(!bDir && fs0.ftime.tmAccess != fs1.ftime.tmAccess)
					visitor.event(FEv_Access(base, path8, bDir), SPData());
				// ファイル編集時刻チェック => FEv_Modify
				if(fs0.ftime.tmModify != fs1.ftime.tmModify ||
					fs0.ftime.tmCreated != fs1.ftime.tmCreated ||
					fs0.size != fs1.size)
					visitor.event(FEv_Modify(base, path8, bDir), SPData());
				// アトリビュート変更チェック => FEv_Attr
				fs0.ftime = fs1.ftime;
				fs0.size = fs1.size;
				if(fs0 != fs1)
					visitor.event(FEv_Attr(base, path8, bDir), SPData());

				if(bDir)
					VisitDifference(visitor, base, ent.second.get(), itr->second.get(), pblock);
			}
			pblock.popBack();
		}
		for(auto& ent : fd1->fmap) {
			auto itr = fd0->fmap.find(ent.first);
			if(itr == fd0->fmap.end()) {
				pblock <<= ent.first;
				// エントリが作成された -> FEv_Create
				bool bDir = ent.second->isDirectory();
				visitor.event(FEv_Create(base, pblock.plain_utf8(), bDir), SPData());
				if(bDir)
					ThrowCreateEvent(visitor, base, ent.second.get(), pblock);
				pblock.popBack();
			}
		}
	}
}
