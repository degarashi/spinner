#include "misc.hpp"
#include "dir.hpp"

namespace spn {
	// -------------------------- PathBlock --------------------------
	PathBlock::PathBlock() {}
	PathBlock::PathBlock(PathBlock&& p): _path(std::move(p._path)), _segment(std::move(p._segment)), _bAbsolute(p._bAbsolute) {}
	PathBlock::PathBlock(To32Str p): PathBlock() {
		setPath(p);
	}
	template <class Itr, class CB>
	void PathBlock::_ReWriteSC(Itr from, Itr to, char32_t sc, CB cb) {
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
					PathBlock::EOS(U'\0');
	bool PathBlock::_IsSC(char32_t c) {
		return c==U'\\' || c==SC;
	}
	void PathBlock::setPath(To32Str elem) {
		int len = elem.getLength();
		const auto *ptr = elem.getPtr(),
					*ptrE = ptr + len;
		// 分割文字を探し、それに統一しながらsegment数を数える
		// 絶対パスの先頭SCは除く
		if((_bAbsolute = _IsSC(*ptr))) {
			++ptr;
			--len;
		}
		if(_IsSC(*(ptrE-1))) {
			--ptrE;
			--len;
		}
		_path.assign(ptr, ptrE);
		_segment.clear();
		_ReWriteSC(_path.begin(), _path.end(), SC, [this](int n){ _segment.push_back(n); });
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
		if(src == srcE)
			return;
		if(_IsSC(*src))
			++src;
		if(_IsSC(*(srcE-1)))
			--srcE;

		if(!_path.empty())
			_path.push_back(SC);
		_path.insert(_path.end(), src, srcE);
		_ReWriteSC(_path.begin()+(srcE-src), _path.end(), SC, [this](int n){ _segment.push_back(n); });
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
	void PathBlock::pushFront(To32Str elem) {
		auto *src = elem.getPtr(),
			*srcE = src + elem.getLength();
		if(src == srcE)
			return;
		if((_bAbsolute = _IsSC(*src)))
			++src;
		if(_IsSC(*(srcE-1)))
			--srcE;

		_path.push_front(SC);
		_path.insert(_path.begin(), src, srcE);
		int ofs = 0;
		_ReWriteSC(_path.begin(), _path.begin()+(srcE-src), SC, [&ofs, this](int n){
			_segment.insert(_segment.begin()+(ofs++), n); });
	}
	void PathBlock::popFront() {
		int sg = segments();
		if(sg > 0) {
			if(sg > 1) {
				_path.erase(_path.begin(), _path.begin()+_segment.front()+1);
				_segment.pop_front();
				_bAbsolute = false;
			} else
				clear();
		}
	}
	std::string PathBlock::plain_utf8(bool bAbs) const {
		return Text::UTFConvertTo8(plain_utf32(bAbs));
	}
	std::u32string PathBlock::plain_utf32(bool bAbs) const {
		std::u32string s;
		if(bAbs && _bAbsolute)
			s.assign(1,SC);
		if(!_path.empty())
			s.append(_path.begin(), _path.end());
		return std::move(s);
	}
	std::string PathBlock::getFirst_utf8(bool bAbs) const {
		return Text::UTFConvertTo8(getFirst_utf32(bAbs));
	}
	std::u32string PathBlock::getFirst_utf32(bool bAbs) const {
		std::u32string s;
		if(bAbs && _bAbsolute)
			s.assign(1,SC);
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

	// -------------------------- Dir --------------------------
	FStatus::FStatus(uint32_t f): flag(f) {}
	FStatus Dir::status() const {
		return _dep.status(plain_utf8());
	}

	Dir::Dir(Dir&& d): PathBlock(std::move(d)), _dep(std::move(d._dep)) {}
	boost::regex Dir::ToRegEx(const std::string& s) {
		// ワイルドカード記述の置き換え
		// * -> ([\/_ \-\w]+)
		// ? -> ([\/_ \-\w])
		// . -> (\.)
		boost::regex re[3] = {boost::regex(R"(\*)"), boost::regex(R"(\?)"), boost::regex(R"(\.)")};
		std::string s2 = boost::regex_replace(s, re[0], R"([\/_ \\-\\w]+)");
		s2 = boost::regex_replace(s2, re[1], R"([\/_ \\-\\w])");
		s2 = boost::regex_replace(s2, re[2], R"(\\.)");
		return boost::regex(s2.c_str());
	}
	Dir::StrList Dir::enumEntryRegEx(const boost::regex& r, bool bRecursive) const {
		StrList res;
		enumEntryRegEx(r, [&res](const PathBlock& pb){ res.push_back(pb.plain_utf8()); }, bRecursive);
		return std::move(res);
	}
	Dir::StrList Dir::enumEntryWildCard(const std::string& s, bool bRecursive) const {
		return enumEntryRegEx(ToRegEx(s), bRecursive);
	}
	void Dir::enumEntryWildCard(const std::string& s, EnumCB cb, bool bRecursive) const {
		enumEntryRegEx(ToRegEx(s), cb, bRecursive);
	}
	void Dir::_enumEntryRegExR(const boost::regex& r, std::string& lpath, size_t baseLen, EnumCB cb) const {
		size_t pl = lpath.size();
		boost::smatch m;
		_dep.enumEntry(lpath, [&, pl, this, baseLen](const char* name) {
			std::string s(name);
			if(s==u8"." || s==u8"..")
				return;
			lpath += '/';
			lpath += s;
			if(_dep.isDirectory(lpath))
				_enumEntryRegExR(r, lpath, baseLen, cb);
			if(boost::regex_match(s, m, r))
				cb(PathBlock(lpath.substr(baseLen)));
			lpath.resize(pl);
		});
	}
	void Dir::_enumEntryRegEx(const boost::regex& r, const std::string& path, EnumCB cb) const {
		boost::smatch m;
		_dep.enumEntry(path, [&](const char* name) {
			std::string s(name);
			if(boost::regex_match(s, m, r))
				cb(PathBlock(s));
		});
	}
	void Dir::enumEntryRegEx(const boost::regex& r, EnumCB cb, bool bRecursive) const {
		auto path = plain_utf8();
		if(bRecursive) {
			_enumEntryRegExR(r, path, path.size()+1, cb);
		} else
			_enumEntryRegEx(r, path, cb);
	}
	void Dir::mkdir(uint32_t mode) const {
		PathReset preset(_dep);
		if(isAbsolute())
			_dep.chdir(u8"/");
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

		if(_dep.isFile(ns)) {
			// パスがファイルだったので失敗とする
			throw std::runtime_error("there is file at the path");
		}
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
		_dep.enumEntry(lpath.plain_utf32(), [&lpath, this, cb](const ChType* name) {
			lpath <<= name;
			if(ChMod(lpath, cb))
				_chmod(lpath, cb);
			lpath.popBack();
		});
	}
	bool Dir::ChMod(const PathBlock& pb, ModCB cb) {
		ToStrType path = pb.plain_utf32();
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
		ToStrType path = plain_utf32();
		return std::fopen(path.getPtr(), mode);
	}
}
