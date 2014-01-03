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
	PathBlock::PathBlock(): _bAbsolute(false) {}
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
	PathBlock::OPChar PathBlock::getDriveLetter() const {
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
	auto PathBlock::_StripSC(Itr from, Itr to) -> OPStripResult<typename std::decay<decltype(*from)>::type>{
		auto* tmp_from = from;
		if(from == to)
			return spn::none;

		StripResult<typename std::decay<decltype(*from)>::type> res;
		if((res.bAbsolute = _IsSC(UTFToN<char32_t>(&*from).code)))
			++from;
		else {
			if(auto dr = _GetDriveLetter(from, to)) {
				res.driveLetter = dr;
				// "C:\"のようになるので3文字分飛ばす
				from += 3;
				res.bAbsolute = true;
				Assert(Throw, from <= to, "invalid path")
			}
		}
		if(from != to) {
			if(_IsSC(UTFToN<char32_t>(&*to).code))
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
				_driveLetter = spn::none;
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
			// windowsの場合のみドライブ文字を出力
			#ifdef WIN32
				if(_driveLetter) {
					dst += *_driveLetter;
					dst += CLN;
				}
			#endif
			dst += SC;
		}
	}
	std::u32string PathBlock::getHeader_utf32() const {
		std::u32string tmp;
		_outputHeader(tmp, true);
		return std::move(tmp);
	}
	std::string PathBlock::getHeader_utf8() const {
		std::u32string tmp;
		_outputHeader(tmp, true);
		return Text::UTFConvertTo8(tmp);
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
		return DirDep::Status(plain_utf8());
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
}
