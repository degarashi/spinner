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

	// ------------------- IFTFile -------------------
	IFTFile::IFTFile(const std::string& name, const FStatus& fs):
		fname(name), fstat(fs) {}
	const std::string& IFTFile::getName() const {
		return fname;
	}
	const FStatus& IFTFile::getStatus() const {
		return fstat;
	}
	bool IFTFile::equal(const IFTFile& f) const {
		return	fname == f.fname &&
				fstat == f.fstat;
	}
	bool IFTFile::operator == (const IFTFile& f) const {
		return equal(f);
	}
	bool IFTFile::operator != (const IFTFile& f) const {
		return !(*this == f);
	}

	// ------------------- FileInfo -------------------
	FileTree::Type FileInfo::getType() const { return FileTree::Type::File; }
	bool FileInfo::isDirectory() const { return false; }
	void FileInfo::print(std::ostream& os, int indent) const {
		InsertIndent(os, indent);
		os << "File: " << fname << std::endl;
	}
	bool FileInfo::equal(const IFTFile& f) const {
		return f.getType() == getType() &&
				IFTFile::equal(f);
	}

	// ------------------- DirInfo -------------------
	FileTree::Type DirInfo::getType() const { return FileTree::Type::Directory; }
	bool DirInfo::isDirectory() const { return true; }
	void DirInfo::print(std::ostream& os, int indent) const {
		InsertIndent(os, indent);
		os << "Dir: " << fname << std::endl;
		for(auto& p : fmap)
			p.second->print(os, indent+1);
	}
	bool DirInfo::equal(const IFTFile& f) const {
		if(f.getType() == getType() &&
			IFTFile::equal(f))
		{
			auto& f2 = static_cast<const DirInfo&>(f);
			auto& fmap2 = f2.fmap;
			if(fmap.size() == fmap2.size()) {
				for(auto& p : fmap) {
					auto itr = fmap2.find(p.first);
					if(itr == fmap2.end() ||
						!p.second->equal(*itr->second))
						return false;
				}
				return true;
			}
		}
		return false;
	}

	#define AS_LPCWSTR(path)	reinterpret_cast<const wchar_t*>(To16Str(path).getStringPtr())
	#define AS_UTF8(path)		To8Str(reinterpret_cast<const char16_t*>(path)).getStringPtr()
	// ------------------- FileTree -------------------
	//! 続きから走査
	UPIFile FileTree::ConstructFTree(Dir& dir) {
		DirInfo* pd;
		UPIFile upd(pd = new DirInfo(dir.getLast_utf8(), dir.status()));

		// 下の階層を読む
		std::string prev = dir.setCurrentDir();
		Dir::EnumEntryWildCard("*", [pd, &dir](const Dir& d) {
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
		Dir::SetCurrentDir(prev);
		return upd;
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
	bool FileTree::VisitDifference(FRecvNotify& visitor, const SPString& base, const IFTFile* f0, const IFTFile* f1, PathBlock& pblock) {
		AssertP(Trap, f0->isDirectory() && f1->isDirectory())
		auto *fd0 = static_cast<const DirInfo*>(f0),
			*fd1 = static_cast<const DirInfo*>(f1);
		// ディレクトリのノード構造が変わった時にtrueにする
		bool bNodeChanged = false;
		for(auto& ent : fd0->fmap) {
			auto itr = fd1->fmap.find(ent.first);
			pblock <<= ent.first;
			bool bDir = ent.second->isDirectory();
			// エントリが削除された時の処理
			auto onRemove = [&](){
				visitor.event(FEv_Remove(base, pblock.plain_utf8(), bDir), SPData());
				// Directoryならば下層のエントリが全部イベント呼び出し対象 -> FEv_Remove
				if(bDir)
					ThrowRemoveEvent(visitor, base, ent.second.get(), pblock);
				bNodeChanged = true;
			};
			if(itr == fd1->fmap.end()) {
				onRemove();
			} else {
				bool bDir2 = itr->second->isDirectory();
				std::string path8(pblock.plain_utf8());
				// 同名のエントリがFile -> Directory等と種別が変わっていたらRemove, Createと解釈
				if(bDir != bDir2) {
					onRemove();
					visitor.event(FEv_Create(base, path8, bDir2), SPData());
					if(bDir2) {
						// 下層のディレクトリは全てCreateを呼ぶ
						ThrowCreateEvent(visitor, base, itr->second.get(), pblock);
					}
				} else {
					FStatus fs0 = ent.second->getStatus();
					const FStatus& fs1 = itr->second->getStatus();
					// アクセス時刻チェック => FEv_Access
					// ディレクトリのアクセスチェックはしない
					if(!bDir) {
						if(fs0.ftime.tmAccess != fs1.ftime.tmAccess)
							visitor.event(FEv_Access(base, path8, bDir), SPData());
						// ファイル編集時刻チェック => FEv_Modify
						if(fs0.ftime.tmModify != fs1.ftime.tmModify ||
							fs0.ftime.tmCreated != fs1.ftime.tmCreated ||
							fs0.size != fs1.size)
							visitor.event(FEv_Modify(base, path8, bDir), SPData());
					}
					// アトリビュート変更チェック => FEv_Attr
					fs0.ftime = fs1.ftime;
					fs0.size = fs1.size;
					if(fs0 != fs1)
						visitor.event(FEv_Attr(base, path8, bDir), SPData());

					if(bDir) {
						// 下層のディレクトリ比較
						bNodeChanged |= VisitDifference(visitor, base, ent.second.get(), itr->second.get(), pblock);
					}
				}
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
				bNodeChanged = true;
			}
		}
		if(bNodeChanged)
			visitor.event(FEv_Modify(base, pblock.plain_utf8(), true), SPData());
		return bNodeChanged;
	}
}
