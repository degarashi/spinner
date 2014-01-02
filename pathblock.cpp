#include "misc.hpp"
#include "dir.hpp"

namespace spn {
	namespace {
		constexpr int UnixEpocYear = 1970,
					WinEpocYear = 1601;
		constexpr uint64_t Second = uint64_t(100 * 1000000),
							Minute = Second * 60,
							Hour = Minute * 60,
							Day = Hour * 24,
							Year = Day * 365;
	}
	uint64_t UnixTime2WinTime(time_t t) {
		uint64_t tmp = t * Second;
		tmp -= (UnixEpocYear - WinEpocYear) * Year;
		return tmp;
	}
	time_t WinTime2UnixTime(uint64_t t) {
		t += (UnixEpocYear - WinEpocYear) * Year;
		t /= Second;
		return static_cast<time_t>(t);
	}
	uint64_t u32_u64(uint32_t valH, uint32_t valL) {
		uint64_t ret = valH;
		ret <<= 32;
		ret |= valL;
		return ret;
	}

	// -------------------------- PathBlock --------------------------
	PathBlock::PathBlock(): _bAbsolute(false), _driveLetter('\0') {}
	PathBlock::PathBlock(PathBlock&& p):
		_path(std::move(p._path)),
		_segment(std::move(p._segment)),
		_bAbsolute(p._bAbsolute)
	{
		p.clear();
	}
	PathBlock::PathBlock(To32Str p): PathBlock() {
		setPath(p);
	}
	char PathBlock::getDriveLetter() const {
		return _driveLetter;
	}
	bool PathBlock::operator == (const PathBlock& p) const {
		return _path == p._path &&
				_segment == p._segment &&
				_bAbsolute == p._bAbsolute;
	}
	bool PathBlock::operator != (const PathBlock& p) const {
		return !(this->operator == (p));
	}
	PathBlock& PathBlock::operator = (PathBlock&& p) {
		_path = std::move(p._path);
		_segment = std::move(p._segment);
		_bAbsolute = p._bAbsolute;
		p.clear();
		return *this;
	}
	template <class CB>
	void PathBlock::_ReWriteSC(Path::iterator from, Path::iterator to, char32_t sc, CB cb) {
		int count = 0;
		while(from != to) {
			if(_IsSC(*from)) {
				*from = sc;
				cb(count);
				count = 0;
			} else
				++count;
			++from;
		}
		if(count > 0)
			cb(count);
	}
	const char32_t PathBlock::SC(U'/'),
					PathBlock::DOT(U'.'),
					PathBlock::EOS(U'\0'),
					PathBlock::CLN(U':');
	bool PathBlock::_IsSC(char32_t c) {
		return c==U'\\' || c==SC;
	}
	void PathBlock::setPath(To32Str elem) {
		int len = elem.getLength();
		const auto *ptr = elem.getPtr(),
					*ptrE = ptr + len;
		// 分割文字を探し、それに統一しながらsegment数を数える
		// 絶対パスの先頭SCは除く

		if(auto res = _StripSC(ptr, ptrE)) {
			_bAbsolute = res->bAbsolute;
			_driveLetter = res->driveLetter;
			ptr += res->nread;

			_path.assign(ptr, ptrE);
			_segment.clear();
			_ReWriteSC(_path.begin(), _path.end(), SC, [this](int n){ _segment.push_back(n); });
		} else
			clear();
	}
	bool PathBlock::isAbsolute() const {
		return _bAbsolute;
	}
	PathBlock& PathBlock::operator <<= (To32Str elem) {
		pushBack(elem);
		return *this;
	}
	void PathBlock::pushBack(To32Str elem) {
		auto *src = elem.getPtr(),
			*srcE = src + elem.getLength();
		if(auto res = _StripSC(src, srcE)){
			Assert(Throw, !res->bAbsolute)
			src += res->nread;

			if(!_path.empty())
				_path.push_back(SC);
			_path.insert(_path.end(), src, srcE);
			_ReWriteSC(_path.end()-(srcE-src), _path.end(), SC, [this](int n){ _segment.push_back(n); });
		}
	}
	void PathBlock::popBack() {
		int sg = segments();
		if(sg > 0) {
			if(sg > 1) {
				_path.erase(_path.end()-_segment.back()-1, _path.end());
				_segment.pop_back();
			} else
				clear();
		}
	}
	template <class Itr>
	auto PathBlock::_GetDriveLetter(Itr from, Itr to) -> spn::Optional<typename std::decay<decltype(*from)>::type> {
		using CH = typename std::decay<decltype(*from)>::type;
		auto cnvToCh = [](char c) {
			char32_t c2 = static_cast<char32_t>(c);
			return UTFToN<CH>(reinterpret_cast<const char*>(&c2)).code;
		};
		Itr tmp_from = from;
		if(to - from >= 3) {
			auto c = *from;
			CH cA = cnvToCh('A'),
				cZ = cnvToCh('Z'),
				ca = cnvToCh('a'),
				cz = cnvToCh('z'),
				col = cnvToCh(':');
			if((c >= cA && c <= cZ) ||
					(c >= ca && c <= cz))
			{
				if(*(++from) == col &&
					_IsSC(UTFToN<char32_t>(&*(++from)).code))
					return *tmp_from;
			}
		}
		return cnvToCh('\0');
	}
	PathBlock::OPStripResult PathBlock::_StripSC(const char32_t* from, const char32_t* to) {
		auto* tmp_from = from;
		if(from == to)
			return spn::none;

		StripResult res;
		if((res.bAbsolute = _IsSC(*from)))
			++from;
		else {
			if(auto dr = _GetDriveLetter(from, to)) {
				res.driveLetter = Text::UTF32To8(*dr).code;
				// "C:\"のようになるので3文字分飛ばす
				from += 3;
				res.bAbsolute = true;
				Assert(Throw, from <= to, "invalid path")
			}
		}
		if(from != to) {
			if(_IsSC(*to))
				--to;
			if(from == to)
				return spn::none;
		}
		res.nread = from - tmp_from;
		return res;
	}
	void PathBlock::pushFront(To32Str elem) {
		Assert(Trap, !_bAbsolute || empty())
		auto *src = elem.getPtr(),
			*srcE = src + elem.getLength();
		if(auto res = _StripSC(src, srcE)) {
			_bAbsolute = res->bAbsolute;
			_driveLetter = res->driveLetter;
			src += res->nread;

			if(src != srcE) {
				_path.push_front(SC);
				_path.insert(_path.begin(), src, srcE);
				int ofs = 0;
				_ReWriteSC(_path.begin(), _path.begin()+(srcE-src), SC, [&ofs, this](int n){
					_segment.insert(_segment.begin()+(ofs++), n); });
			}
		}
	}
	void PathBlock::popFront() {
		int sg = segments();
		if(sg > 0) {
			if(sg > 1) {
				_path.erase(_path.begin(), _path.begin()+_segment.front()+1);
				_segment.pop_front();
				_bAbsolute = false;
				_driveLetter = '\0';
			} else
				clear();
		}
	}
	std::string PathBlock::plain_utf8(bool bAbs) const {
		return Text::UTFConvertTo8(plain_utf32(bAbs));
	}
	std::u32string PathBlock::plain_utf32(bool bAbs) const {
		std::u32string s;
		_outputHeader(s, bAbs);
		if(!_path.empty())
			s.append(_path.begin(), _path.end());
		return std::move(s);
	}
	std::string PathBlock::getFirst_utf8(bool bAbs) const {
		return Text::UTFConvertTo8(getFirst_utf32(bAbs));
	}
	std::string PathBlock::getSegment_utf8(int beg, int end) const {
		return Text::UTFConvertTo8(getSegment_utf32(beg, end));
	}
	void PathBlock::_outputHeader(std::u32string& dst, bool bAbs) const {
		if(bAbs && _bAbsolute) {
			if(_driveLetter) {
				dst += *_driveLetter;
				dst += CLN;
			}
			dst += SC;
		}
	}
	std::u32string PathBlock::getFirst_utf32(bool bAbs) const {
		std::u32string s;
		_outputHeader(s, bAbs);
		if(!_path.empty()) {
			s.append(_path.begin(),
					_path.begin() + _segment.front());
		}
		return std::move(s);
	}
	std::string PathBlock::getLast_utf8() const {
		return Text::UTFConvertTo8(getLast_utf32());
	}
	std::u32string PathBlock::getLast_utf32() const {
		std::u32string s;
		if(!_path.empty()) {
			s.append(_path.end()-_segment.back(),
					_path.end());
		}
		return std::move(s);
	}
	std::u32string PathBlock::getSegment_utf32(int beg, int end) const {
		std::u32string s;
		if(!_path.empty()) {
			beg = std::max(0, beg);
			end = std::min(int(_segment.size()), end);
			if(beg < end) {
				int ofs_b = 0,
					ofs_e = 0;
				for(int i=0 ; i<beg ; i++)
					ofs_b += _segment[i] + 1;
				for(int i=0 ; i<end ; i++)
					ofs_e += _segment[i] + 1;
				ofs_e = spn::Saturate(ofs_e, 0, int(_path.size()));

				s.append(_path.begin() + ofs_b,
						 _path.begin() + ofs_e);
				if(!s.empty() && s.back()==SC)
					s.pop_back();
			}
		}
		return std::move(s);
	}
	int PathBlock::size() const {
		return _path.size() + (isAbsolute() ? 1 : 0);
	}
	int PathBlock::segments() const {
		return _segment.size();
	}
	void PathBlock::clear() {
		_path.clear();
		_segment.clear();
		_bAbsolute = false;
	}
	bool PathBlock::empty() const {
		return _path.empty();
	}
	std::string PathBlock::getExtension(bool bRaw) const {
		std::string rt;
		if(segments() > 0) {
			auto ts = getLast_utf32();
			const auto* str = ts.c_str();
			char32_t tc[64];
			int wcur = 0;
			bool bFound = false;
			for(;;) {
				auto c = *str++;
				if(c == EOS) {
					if(bFound) {
						tc[wcur] = EOS;
						if(bRaw) {
							// 末尾の数字を取り去る
							--wcur;
							while(wcur>=0 && std::isdigit(static_cast<char>(tc[wcur])))
								tc[wcur--] = EOS;
						}
						rt = Text::UTFConvertTo8(tc);
					}
					break;
				} else if(c == DOT)
					bFound = true;
				else if(bFound)
					tc[wcur++] = c;
			}
		}
		return std::move(rt);
	}
	int PathBlock::getExtNum() const {
		if(segments() > 0) {
			auto ts = getLast_utf8();
			return _ExtGetNum(ts);
		}
		return -1;
	}
	int PathBlock::_ExtGetNum(const std::string& ext) {
		int wcur = 0;
		const auto* str = ext.c_str();
		for(;;) {
			auto c = *str++;
			if(std::isdigit(c)) {
				// 数字部分を取り出す
				return std::atol(str-1);
			} else if(c == '\0')
				break;
		}
		return 0;
	}
	int PathBlock::addExtNum(int n) {
		if(segments() > 0) {
			auto ts = getExtension(false);
			n = _ExtIncNum(ts, n);
			setExtension(std::move(ts));
			return n;
		}
		return -1;
	}
	void PathBlock::setExtension(To32Str ext) {
		if(segments() > 0) {
			auto ts = getLast_utf32();
			const auto* str = ts.c_str();
			int rcur = 0;
			while(str[rcur]!=EOS && str[rcur++]!=DOT);
			if(str[rcur] == EOS) {
				// 拡張子を持っていない
				ts += DOT;
			} else {
				// 拡張子を持っている
				ts.resize(rcur);
			}
			ts.append(ext.getPtr(), ext.getPtr()+ext.getLength());
			popBack();
			pushBack(std::move(ts));
		}
	}
	int PathBlock::_ExtIncNum(std::string& ext, int n) {
		int wcur = 0,
		rcur = 0;
		char tc[32] = {};
		const auto* str = ext.c_str();
		for(;;) {
			auto c = str[rcur];
			if(std::isdigit(c)) {
				// 数字部分を取り出す
				n += std::atol(str+rcur);
				ext.resize(rcur);
				std::sprintf(tc, "%d", n);
				ext += tc;
				return n;
			} else if(c == '\0')
				break;
			++rcur;
		}
		std::sprintf(tc, "%d", n);
		ext += tc;
		return n;
	}

	// ------------------- FStatus -------------------
	FStatus::FStatus(uint32_t f): flag(f) {}
	FStatus Dir::status() const {
		return _dep.status(plain_utf8());
	}
	bool FStatus::operator == (const FStatus& f) const {
		return flag == f.flag &&
			userId == f.userId &&
			groupId == f.groupId &&
			size == f.size &&
			ftime == f.ftime;
	}
	bool FStatus::operator != (const FStatus& f) const {
		return !(*this == f);
	}
	// ------------------- FTime -------------------
	bool FTime::operator == (const FTime& ft) const {
		return X_OrArgs(tmCreated, ft.tmCreated,
						tmAccess, ft.tmAccess,
						tmModify, ft.tmModify) == 0;
	}
	bool FTime::operator != (const FTime& ft) const {
		return !(*this == ft); }

	// -------------------------- Dir --------------------------
	const char Dir::SC('/'),
				Dir::DOT('.'),
				Dir::EOS('\0'),
				*Dir::SC_P(u8"/"),
				Dir::LBK('['),
				Dir::RBK(']');
	Dir::Dir(Dir&& d): PathBlock(std::move(d)), _dep(std::move(d._dep)) {}
	Dir& Dir::operator = (Dir&& d) {
		static_cast<PathBlock&>(*this) = std::move(static_cast<PathBlock&>(d));
		_dep = std::move(d._dep);
		return *this;
	}
	std::string Dir::GetCurrentDir() {
		return DirDep::GetCurrentDir();
	}
	std::string Dir::ToRegEx(const std::string& s) {
		// ワイルドカード記述の置き換え
		// * -> ([\/_ \-\w]+)
		// ? -> ([\/_ \-\w])
		// . -> (\.)
		boost::regex re[3] = {boost::regex(R"(\*)"), boost::regex(R"(\?)"), boost::regex(R"(\.)")};
		std::string s2 = boost::regex_replace(s, re[0], R"([\/_ \\-\\w]+)");
		s2 = boost::regex_replace(s2, re[1], R"([\/_ \\-\\w])");
		s2 = boost::regex_replace(s2, re[2], R"(\\.)");
		return std::move(s2);
	}
	Dir::StrList Dir::enumEntryRegEx(const std::string& r) const {
		StrList res;
		enumEntryRegEx(r, [&res](const Dir& dir){
			res.push_back(dir.plain_utf8());
		});
		return std::move(res);
	}
	Dir::StrList Dir::enumEntryWildCard(const std::string& s) const {
		assert(_path.empty() || !PathBlock(s).isAbsolute());
		return enumEntryRegEx(ToRegEx(s));
	}
	void Dir::enumEntryWildCard(const std::string& s, EnumCB cb) const {
		enumEntryRegEx(ToRegEx(s), cb);
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
					if(itr0 != itr)
						rl.emplace_back(itr0, itr);
					itr0 = ++itr;
					continue;
				}
			}
			++itr;
		}
		if(itr0 != itr)
			rl.emplace_back(itr0, itr);
		return std::move(rl);
	}
	void Dir::_enumEntryRegEx(RegexItr itr, RegexItr itrE, std::string& lpath, size_t baseLen, EnumCB cb) const {
		if(itr == itrE)
			return;

		size_t pl = lpath.size();
		_dep.enumEntry(lpath, [=, &lpath, &cb](const PathCh* name, bool) {
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
				if(_dep.isDirectory(ToPathStr(lpath))) {
					if(itr+1 != itrE)
						_enumEntryRegEx(itr+1, itrE, lpath, baseLen, cb);
					else
						cb(Dir(lpath));
				} else
					cb(Dir(lpath));
				lpath.resize(pl);
			}
		});
	}
	void Dir::_enumEntry(const std::string& s, const std::string& path, EnumCB cb) const {
		_dep.enumEntry(path, [&cb](const PathCh* name, bool bDir) {
			PathStr s(ToPathStr(name).moveTo());
			if(s == name)
				cb(Dir(s));
		});
	}
	void Dir::enumEntryRegEx(const std::string& r, EnumCB cb) const {
		if(r.empty())
			return;
		std::string path;
		// 絶対パスの時は内部パスを無視する
		if(_IsSC(Text::UTF8To32(r[0]).code))
			path = '/';
		else
			path = plain_utf8();
		try {
			RegexL rl = _ParseRegEx(r);
			_enumEntryRegEx(rl.begin(), rl.end(), path, path.size()+1, cb);
		} catch(const boost::regex_error& e) {
			// 正規表現に何かエラーがある時は単純に文字列比較とする
			_enumEntry(r, path, cb);
		}
	}
	bool Dir::isFile() const {
		return _dep.isFile(plain_utf32());
	}
	bool Dir::isDirectory() const {
		return _dep.isDirectory(plain_utf32());
	}
	void Dir::remove() const {
		_dep.remove(plain_utf32());
	}
	void Dir::copy(const std::string& to) const {
		_dep.copy(plain_utf32(), to);
	}
	void Dir::move(const std::string& to) const {
		_dep.move(plain_utf32(), to);
	}
	void Dir::mkdir(uint32_t mode) const {
		PathReset preset(_dep);
		if(isAbsolute())
			_dep.chdir(SC_P);
		mode |= FStatus::UserRWX;

		auto path32 = plain_utf32(false);
		const char32_t* ptr = &path32[0];
		int nsg = segments();
		std::string ns;
		int i;
		// 最初のパスから1つずつ存在確認
		for(i=0 ; i<nsg ; i++) {
			int seg = _segment[i];
			ns = Text::UTFConvertTo8(c32Str(ptr, seg));
			ptr += seg+1;
			if(!_dep.chdir_nt(ns))
				break;
		}
		if(i == nsg)
			return;

		// パスがファイルだったら失敗とする
		Assert(Throw, !_dep.isFile(ns), "there is file at the path")
		for(;;) {
			_dep.mkdir(ns, mode);
			_dep.chdir(ns);
			if(++i == nsg)
				break;
			int seg = _segment[i] + 1;
			ns = Text::UTFConvertTo8(c32Str(ptr, seg));
			ptr += seg;
		}
	}
	void Dir::_chmod(PathBlock& lpath, ModCB cb) {
		_dep.enumEntry(lpath.plain_utf32(), [&lpath, this, cb](const PathCh* name, bool) {
			lpath <<= name;
			if(ChMod(lpath, cb))
				_chmod(lpath, cb);
			lpath.popBack();
		});
	}
	bool Dir::ChMod(const PathBlock& pb, ModCB cb) {
		ToPathStr path = pb.plain_utf32();
		FStatus fstat = _dep.status(path);
		bool bDir = fstat.flag & FStatus::DirectoryType;
		if(bDir)
			fstat.flag |= FStatus::UserExec;
		bool bRecr = cb(pb, fstat);
		_dep.chmod(path, fstat.flag);
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

	// -------------------------- URI --------------------------
	const std::string URI::SEP(u8"://");
	const std::u32string URI::SEP32(U"://");
	URI::URI() {}
	URI::URI(URI&& u): PathBlock(std::move(u)), _type(std::move(u._type)) {}
	URI::URI(To8Str p) {
		setPath(p);
	}
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
			PathBlock::setPath(p);
	}
	const std::string& URI::getType_utf8() const {
		return _type;
	}
	void URI::setType(const std::string& typ) {
		_type = typ;
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
		return std::move(ret);
	}
	std::u32string URI::plainUri_utf32() const {
		auto ret = To32Str(_type).moveTo();
		ret.append(SEP32);
		ret.append(plain_utf32());
		return std::move(ret);
	}
}
