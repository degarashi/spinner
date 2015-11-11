#include "dir.hpp"

namespace spn {
	// -------------------------- Dir --------------------------
	const char Dir::SC('/'),
				Dir::DOT('.'),
				Dir::EOS('\0'),
				*Dir::SC_P(u8"/"),
				Dir::LBK('['),
				Dir::RBK(']');
	Dir::Dir(Dir&& d): PathBlock(std::move(d)) {}
	Dir& Dir::operator = (Dir&& d) {
		static_cast<PathBlock&>(*this) = std::move(static_cast<PathBlock&>(d));
		return *this;
	}
	std::string Dir::GetCurrentDir() {
		return To8Str(DirDep::GetCurrentDir()).moveTo();
	}
	std::string Dir::GetProgramDir() {
		return To8Str(DirDep::GetProgramDir()).moveTo();
	}
	void Dir::SetCurrentDir(const std::string& path) {
		PathStr ps;
		// Windows環境において先頭のドライブ文字を削る
		if(path.length() > 1 &&
			::isalpha(path[0]) && path[1] == ':')
		{
			ps = ToPathStr(path.substr(2)).moveTo();
		} else
			ps = ToPathStr(path).moveTo();
		DirDep::SetCurrentDir(ps);
	}
	std::string Dir::ToRegEx(const std::string& s) {
		// ワイルドカード記述の置き換え
		// * -> ([_ \-\w]+)
		// ? -> ([_ \-\w])
		// . -> (\.)

		// バックスラッシュをスラッシュに置き換え
		// \ -> /
		boost::regex re[4] = {boost::regex(R"(\\)"), boost::regex(R"(\*)"), boost::regex(R"(\?)"), boost::regex(R"(\.)")};
		std::string s2 = boost::regex_replace(s, re[0], R"(/)");
		s2 = boost::regex_replace(s2, re[1], R"([_ \\-\\w]+)");
		s2 = boost::regex_replace(s2, re[2], R"([_ \\-\\w])");
		s2 = boost::regex_replace(s2, re[3], R"(\\.)");
		return s2;
	}
	std::string Dir::setCurrentDir() const {
		std::string prev = GetCurrentDir();
		SetCurrentDir(plain_utf8());
		return prev;
	}
	Dir::StrList Dir::EnumEntryRegEx(const std::string& r) {
		StrList res;
		EnumEntryRegEx(r, [&res](const Dir& dir){
			res.push_back(dir.plain_utf8());
		});
		return res;
	}
	Dir::StrList Dir::EnumEntryWildCard(const std::string& s) {
		return EnumEntryRegEx(ToRegEx(s));
	}
	void Dir::EnumEntryWildCard(const std::string& s, EnumCB cb) {
		EnumEntryRegEx(ToRegEx(s), cb);
	}
	Dir::RegexL Dir::_ParseRegEx(const std::string& r) {
		RegexL rl;
		auto itr = r.begin(),
			itrE = r.end(),
			itr0 = itr;
		bool bSkip = false;
		while(itr != itrE) {
			auto c = *itr;
			if(bSkip) {
				if(c == RBK)
					bSkip = false;
			} else {
				if(c == LBK)
					bSkip = true;
				else if(c == SC) {
					auto diff = itr - itr0;
					bool bIgnore = false;
					if(diff == 0)
						bIgnore = true;
					else if(diff >= 2) {
						if(*itr0 == '\\' && *(itr0+1) == '.') {
							if(diff == 2) {
								// セグメントをスキップ
								bIgnore = true;
							} else if(diff == 4 && (*(itr0+2) == '\\' && *(itr0+3) == '.')) {
								// セグメントを1つ戻す
								Assert(Trap, !rl.empty())
								rl.pop_back();
								bIgnore = true;
							}
						}
					}
					if(!bIgnore)
						rl.emplace_back(itr0, itr);
					itr0 = ++itr;
					continue;
				}
			}
			++itr;
		}
		if(itr0 != itr)
			rl.emplace_back(itr0, itr);
		return rl;
	}
	void Dir::_EnumEntryRegEx(RegexItr itr, RegexItr itrE, std::string& lpath, size_t baseLen, EnumCB cb) {
		if(itr == itrE)
			return;

		size_t pl = lpath.size();
		DirDep::EnumEntry(lpath, [=, &lpath, &cb](const PathCh* name, bool) {
			if(name[0]==PathCh(DOT)) {
				if(name[1]==PathCh(EOS) || name[1]==PathCh(DOT))
					return;
			}
			std::string s(To8Str(name).moveTo());
			boost::smatch m;
			if(boost::regex_match(s, m, *itr)) {
				if(lpath.back() != SC)
					lpath += SC;
				lpath += s;
				if(DirDep::IsDirectory(ToPathStr(lpath))) {
					if(itr+1 != itrE)
						_EnumEntryRegEx(itr+1, itrE, lpath, baseLen, cb);
					else
						cb(Dir(lpath));
				} else
					cb(Dir(lpath));
				lpath.resize(pl);
			}
		});
	}
	void Dir::_EnumEntry(const std::string& /*s*/, const std::string& path, EnumCB cb) {
		DirDep::EnumEntry(path, [&cb](const PathCh* name, bool /*bDir*/) {
			PathStr s(ToPathStr(name).moveTo());
			if(s == name)
				cb(Dir(s));
		});
	}
	void Dir::EnumEntryRegEx(const std::string& r, EnumCB cb) {
		if(r.empty())
			return;
		std::string path;
		// 絶対パスの時は内部パスを無視する
		int ofs = 0;
		bool bAbs = false;
		if(r[0] == '/') {
			#ifdef WIN32
				Assert(Throw, false, "invalid absolute path")
			#endif
			path += '/';
			ofs = 1;
			bAbs = true;
		} else if(auto letter = _GetDriveLetter(&r[0], &r[0] + r.length())) {
			// windowsの場合のみドライブ文字を出力
			#ifdef WIN32
				path += *letter;
				path += ':';
			#else
				path += '/';
			#endif
			ofs = 2;
			bAbs = true;
		}
		if(!bAbs)
			path += "./";

		try {
			RegexL rl = _ParseRegEx(r.substr(ofs));
			_EnumEntryRegEx(rl.begin(), rl.end(), path, path.size()+1, cb);
		} catch(const boost::regex_error& e) {
			// 正規表現に何かエラーがある時は単純に文字列比較とする
			_EnumEntry(r, path, cb);
		}
	}
	bool Dir::isFile() const {
		return DirDep::IsFile(plain_utf32());
	}
	bool Dir::isDirectory() const {
		return DirDep::IsDirectory(plain_utf32());
	}
	void Dir::remove() const {
		DirDep::Remove(plain_utf32());
	}
	void Dir::copy(const std::string& to) const {
		DirDep::Copy(plain_utf32(), to);
	}
	void Dir::move(const std::string& to) const {
		DirDep::Move(plain_utf32(), to);
	}
	void Dir::mkdir(uint32_t mode) const {
		PathReset preset;
		if(isAbsolute())
			DirDep::Chdir(SC_P);
		mode |= FStatus::UserRWX;

		int nsg = segments();
		std::string ns;
		int i;
		// 最初のパスから1つずつ存在確認
		for(i=0 ; i<nsg ; i++) {
			ns = getSegment_utf8(i,i+1);
			if(!DirDep::Chdir_nt(ns))
				break;
		}
		if(i == nsg)
			return;

		// パスがファイルだったら失敗とする
		Assert(Throw, !DirDep::IsFile(ns), "there is file at the path")
		for(;;) {
			DirDep::Mkdir(ns, mode);
			DirDep::Chdir(ns);
			if(++i == nsg)
				break;
			ns = getSegment_utf8(i,i+1);
		}
	}
	void Dir::_chmod(PathBlock& lpath, ModCB cb) {
		DirDep::EnumEntry(lpath.plain_utf32(), [&lpath, this, cb](const PathCh* name, bool) {
			lpath <<= name;
			if(ChMod(lpath, cb))
				_chmod(lpath, cb);
			lpath.popBack();
		});
	}
	bool Dir::ChMod(const PathBlock& pb, ModCB cb) {
		ToPathStr path = pb.plain_utf32();
		FStatus fstat = DirDep::Status(path);
		bool bDir = fstat.flag & FStatus::DirectoryType;
		if(bDir)
			fstat.flag |= FStatus::UserExec;
		bool bRecr = cb(pb, fstat);
		DirDep::Chmod(path, fstat.flag);
		return bDir && bRecr;
	}
	void Dir::chmod(ModCB cb) {
		PathBlock pb(*this);
		if(ChMod(pb, cb))
			_chmod(pb, cb);
	}
	void Dir::chmod(uint32_t mode) {
		chmod([mode](const PathBlock&, FStatus& fs) {
			fs.flag = mode;
			return false;
		});
	}
	FILE* Dir::openAsFP(const char* mode) const {
		return std::fopen(To8Str(plain_utf32()).getStringPtr(), mode);
	}
	FStatus Dir::status() const {
		return DirDep::Status(plain_utf8());
	}
	void Dir::RemoveAll(const std::string& path) {
		if(DirDep::IsDirectory(path)) {
			auto prev = Dir::GetCurrentDir();
			Dir::SetCurrentDir(path);
			// 下層のファイルやディレクトリを削除
			EnumEntryWildCard("*", [](const Dir& d){
				RemoveAll(d.plain_utf8(true));
			});
			DirDep::Rmdir(path);
			Dir::SetCurrentDir(prev);
		} else
			DirDep::Remove(path);
	}
	std::string Dir::plain_utf8(bool bAbs) const {
		if(bAbs) {
			if(!PathBlock::isAbsolute()) {
				PathBlock pb(GetCurrentDir());
				pb <<= static_cast<const PathBlock&>(*this);
				return pb.plain_utf8(true);
			}
		}
		return PathBlock::plain_utf8(true);
	}
	std::u32string Dir::plain_utf32(bool bAbs) const {
		if(bAbs) {
			if(!PathBlock::isAbsolute()) {
				PathBlock pb(GetCurrentDir());
				pb <<= static_cast<const PathBlock&>(*this);
				return pb.plain_utf32(true);
			}
		}
		return PathBlock::plain_utf32(true);
	}

	// -------------------------- URI --------------------------
	const std::string URI::SEP(u8"://");
	const std::u32string URI::SEP32(U"://");
	URI::URI() {}
	URI::URI(URI&& u): PathBlock(std::move(u)), _type(std::move(u._type)) {}
	URI::URI(To8Str p) {
		setPath(p);
	}
	URI::URI(To8Str typ, To8Str path): PathBlock(path), _type(typ.moveTo()) {}
	URI::URI(To8Str typ, const PathBlock& pb): PathBlock(pb), _type(typ.moveTo()) {}

	URI& URI::operator = (URI&& u) {
		static_cast<PathBlock&>(*this) = std::move(u);
		_type = std::move(u._type);
		return *this;
	}
	void URI::setPath(To8Str p) {
		std::string path(p.moveTo());
		boost::regex re("^([\\w\\d_]+)://");
		boost::smatch m;
		if(boost::regex_search(path, m, re)) {
			_type = m.str(1);
			PathBlock::setPath(path.substr(m[0].length()));
		} else
			PathBlock::setPath(std::move(path));
	}
	const std::string& URI::getType_utf8() const {
		return _type;
	}
	void URI::setType(To8Str typ) {
		_type = typ.moveTo();
	}
	const PathBlock& URI::path() const {
		return *this;
	}
	PathBlock& URI::path() {
		return *this;
	}
	std::string URI::plainUri_utf8() const {
		auto ret = _type;
		ret.append(SEP);
		ret.append(plain_utf8());
		return ret;
	}
	std::u32string URI::plainUri_utf32() const {
		auto ret = To32Str(_type).moveTo();
		ret.append(SEP32);
		ret.append(plain_utf32());
		return ret;
	}
	bool URI::operator == (const URI& u) const {
		return _type == u._type
				&& static_cast<const PathBlock&>(*this) == u;
	}
	bool URI::operator != (const URI& u) const {
		return !(this->operator == (u));
	}
}
