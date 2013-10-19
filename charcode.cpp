#include "misc.hpp"
#include <locale>
namespace spn {
	StrLen GetLength(const char* str) {
		return Text::utf8_strlen(str);
	}
	StrLen GetLength(const char16_t* str) {
		return Text::utf16_strlen(str);
	}
	StrLen GetLength(const char32_t* str) {
		size_t count = 0;
		while(*str != U'\0')
			++count;
		return count;
	}

	bool Text::sjis_isMBChar(char c) {
		uint32_t subset = c & 0xff;
		if((subset >= 0x81 && subset <= 0x9F) || (subset >= 0xe0))
			return true;
		return false;
	}
	int Text::sjis_strlen(const char* str) {
		// 文字列バイト数
		int nStr = std::strlen(str);
		int result = nStr;
		for(int i=0 ; i<nStr ; i++) {
			if(sjis_isMBChar(str[i])) {
				result --;
				i ++;
			}
		}
		return result;
	}
	std::pair<Text::CODETYPE, int> Text::GetEncodeType(const void* ptr) {
		auto* str = reinterpret_cast<const uint8_t*>(ptr);
		auto bom = *((const uint32_t*)ptr);
		if(bom == 0xfffe0000)
			return std::make_pair(CODETYPE::UTF_32BE, 4);
		if(bom == 0x0000feff)
			return std::make_pair(CODETYPE::UTF_32LE, 4);
		if((bom&0x00ffffff) == 0x00bfbbef)
			return std::make_pair(CODETYPE::UTF_8, 3);
		bom &= 0xffff;
		if(bom == 0xfffe)
			return std::make_pair(CODETYPE::UTF_16BE, 2);
		if(bom == 0xfeff)
			return std::make_pair(CODETYPE::UTF_16LE, 2);
		return std::make_pair(CODETYPE::ANSI, 0);
	}
	uint32_t Text::CharToHex(uint32_t ch) {
		if(ch >= 'a')
			return (ch-'a') + 10;
		else if(ch >= 'A')
			return (ch-'A') + 10;
		else
			return ch-'0';
	}
	uint32_t Text::StrToHex4(const char* src) {
		return (CharToHex(src[0])<<12) |
		(CharToHex(src[1])<<8) |
		(CharToHex(src[2])<<4) |
		CharToHex(src[3]);
	}
	uint32_t Text::HexToChar(uint32_t hex) {
		if(hex >= 10)
			return (hex-10) + 'a';
		return hex + '0';
	}

	namespace {
		const char c_base64[64+1] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		const char c_base64_num[128] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62,0, 0, 0, 63,
			52,53,54,55,56,57,58,59,60,61,0, 0, 0, 127,0,0,
			0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,
			15,16,17,18,19,20,21,22,23,24,25,0, 0, 0, 0, 0,
			0, 26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
			41,42,43,44,45,46,47,48,49,50,51,0, 0, 0, 0, 0
		};
	}
	int Text::base64(char* dst, size_t n_dst, const char* src, int n) {
		uint32_t buff=0;
		int nlen=0;

		int rcur = 0,
		wcur = 0;
		for(;n>0;--n) {
			unsigned char c = src[rcur++];

			buff <<= 8;
			buff |= c;
			nlen += 8;

			while(nlen >= 6) {
				dst[wcur++] = c_base64[buff >> (nlen-6)];
				nlen -= 6;
				buff &= (1<<nlen)-1;
			}
		}

		if(nlen > 0) {
			buff <<= (6-nlen);
			dst[wcur++] = c_base64[buff];
		}

		if(wcur & 0x03) {
			int n = 4 - (wcur&0x03);
			for(int i=0 ; i<n ; i++)
				dst[wcur++] = '=';
		}

		if(wcur >= (int)n_dst)
			throw std::length_error("base64(): buffer overflow");

		dst[wcur] = 0;
		return wcur;
	}
	int Text::base64toNum(char* dst, size_t n_dst, const char* src, int n) {
		uint32_t buff=0;
		int nlen=0;
		int wcur=0;

		for(;;) {
			unsigned char c = *src++;
			if(--n < 0 || c == '=')
				break;

			buff <<= 6;
			buff |= c_base64_num[c];
			nlen += 6;

			while(nlen >= 8) {
				nlen -= 8;
				dst[wcur++] = (char)(buff>>nlen);
				buff &= (1<<nlen)-1;
			}
		}
		if(wcur > (int)n_dst)
			throw std::length_error("base64toNum(): buffer overflow");

		return wcur;
	}
	namespace {
		bool IsURLChar_OAUTH(char c) {
			return (c>='A' && c<='Z') || (c>='0' && c<='9') || (c>='a' && c<='z') ||
			c=='.' || c=='-' || c=='_' || c=='~';
		}
		char Get16Char_OAUTH(int num) {
			if(num >= 10)
				return 'A'+num-10;
			return '0'+num;
		}

		bool IsURLChar(char c) {
			return (c>='A' && c<='Z') || (c>='0' && c<='9') || (c>='a' && c<='z') ||
			c=='\'' || c=='.' || c=='-' || c=='*' || c==')' || c=='(' || c=='_';
		}
		char Get16Char(int num) {
			if(num >= 10)
				return 'a'+num-10;
			return '0'+num;
		}
	}
	int Text::url_encode_OAUTH(char* dst, size_t n_dst, const char* src, int n) {
		int wcur = 0,
		rcur = 0;
		for(;;) {
			unsigned char c = src[rcur++];
			if(--n < 0)
				break;
			else if(IsURLChar_OAUTH(c))
				dst[wcur++] = c;
			else {
				dst[wcur++] = '%';
				dst[wcur++] = Get16Char_OAUTH(c>>4);
				dst[wcur++] = Get16Char_OAUTH(c&0x0f);
			}
		}
		dst[wcur] = '\0';
		if(wcur >= (int)n_dst)
			throw std::length_error("url_encode_OAUTH(): buffer overflow");
		return wcur;
	}
	int Text::url_encode(char* dst, size_t n_dst, const char* src, int n) {
		int wcur = 0,
		rcur = 0;
		for(;;) {
			unsigned char c = src[rcur++];
			if(--n < 0)
				break;
			else if(IsURLChar(c))
				dst[wcur++] = c;
			else if(c == ' ')
				dst[wcur++] = '+';
			else {
				dst[wcur++] = '%';
				dst[wcur++] = Get16Char(c>>4);
				dst[wcur++] = Get16Char(c&0x0f);
			}
		}
		dst[wcur] = '\0';
		if(wcur >= (int)n_dst)
			throw std::length_error("url_encode(): buffer overflow");
		return wcur;
	}

	StrLen Text::utf8_strlen(const char* str) {
		auto *cur0 = reinterpret_cast<const uint8_t*>(str),
			*cur = cur0;
		constexpr int invalid = 0xffff;
		const int c_size[] = {1, invalid, 2, 3, 4, 5, 6, invalid};
		size_t count = 0;

		uint8_t c = *cur;
		while(c != '\0') {
			cur += c_size[7 - Bit::MSB_N(uint8_t(~c))];
			++count;
			c = *cur;
		}
		return StrLen(cur-cur0, count);
	}
	bool Text::utf16_isSurrogate(char16_t c) {
		return (c & 0xdc00) == 0xd800;
	}
	StrLen Text::utf16_strlen(const char16_t* str) {
		int cur = 0;
		size_t count = 0;
		char16_t c = str[cur++];
		while(c != L'\0') {
			++count;
			if(Text::utf16_isSurrogate(c))
				++cur;
			c = str[cur++];
		}
		return StrLen(cur, count);
	}
	bool Text::utf16_isSpace(char16_t c) {
		return (c==L' ') || (c==L'　');
	}
	bool Text::utf16_isLF(char16_t c) {
		return c==L'\n';
	}
	bool Text::utf16_isPrivate(char16_t c) {
		return c>=0xe000 && c<0xf900;
	}

	Text::Code Text::UTF16To32(char32_t src) {
		Code ret;
		ret.nwrite = 1;
		if((src & 0xdc00dc00) == 0xdc00d800) {
			// サロゲートペア
			char32_t l_bit = src & 0x03ff0000,
			h_bit = (src & 0x000003ff) + 0x40;

			ret.code = (l_bit>>16)|(h_bit<<10);
			ret.nread = 2;
		} else {
			// 16bit数値
			ret.code = src&0xffff;
			ret.nread = 1;
		}
		return ret;
	}
	Text::Code Text::UTF32To16(char32_t src) {
		Code ret;
		ret.nread = 1;
		if(src >= 0x10000) {
			// サロゲートペア
			char32_t l_bit = (src & 0x3ff)<<16,
			h_bit = ((src>>10)&0x3ff) - 0x40;
			ret.code = h_bit|l_bit|0xdc00d800;
			ret.nwrite = 2;
		} else {
			// 16bit数値
			ret.code = src;
			ret.nwrite = 1;
		}
		return ret;
	}
	Text::Code Text::UTF8To32(char32_t src) {
		Code ret;
		ret.nwrite = 1;
		if((src&0x80) == 0) {
			// 1バイト文字
			ret.code = src&0xff;
			ret.nread = 1;
		} else if((src&0xc0e0) == 0x80c0) {
			// 2バイト文字
			ret.code = ((src&0x1f)<<6) |
					((src&0x3f00)>>8);
			ret.nread = 2;
		} else if((src&0xc0c0f0) == 0x8080e0) {
			// 3バイト文字
			ret.code = ((src&0x0f)<<12) |
					((src&0x3f00)>>2) |
					((src&0x3f0000)>>16);
			ret.nread = 3;
		} else if((src&0xc0c0c0f8) == 0x808080f0) {
			// 4バイト文字
			ret.code = ((src&0x07)<<18) |
					((src&0x3f00)<<4) |
					((src&0x3f0000)>>10) |
					((src&0x3f000000)>>24);
			ret.nread = 4;
		} else
			throw std::invalid_argument("unknown unicode char");
		return ret;
	}
	Text::Code Text::UTF8To32_s(char32_t src) {
		Code ret = UTF8To32(src);
		const char32_t mask[5] = {0, 0,0x1e00,0x0f2000,0x07300000};
		const char32_t ormask[5] = {0, 1,0,0,0};

		if((ret.code & mask[ret.nread]) | ormask[ret.nread]) {}
		else
			throw std::invalid_argument("invalid unicode sequence");
		return ret;
	}
	Text::Code Text::UTF32To8(char32_t src) {
		Code ret;
		ret.nread = 1;
		if(src < 0x80) {
			// 1バイト文字
			ret.code = src;
			ret.nwrite = 1;
		} else if(src < 0x0400) {
			// 2バイト文字
			ret.code = ((0x80 | (src&0x3f))<<8) |
					(0xc0 | ((src&0x07c0)>>6));
					ret.nwrite = 2;
		} else if(src < 0x10000) {
			// 3バイト文字
			ret.code = ((0x80 | (src&0x3f))<<16) |
					((0x80 | ((src&0x0fc0)>>6))<<8) |
					(0xe0 | ((src&0xf000)>>12));
			ret.nwrite = 3;
		} else {
			// 4バイト文字
			ret.code = ((0x80 | (src&0x3f))<<24) |
					((0x80 | ((src&0x0fc0)>>6))<<16) |
					((0x80 | ((src&0x3f000)>>12))<<8) |
					(0xf0 | ((src&0x1c0000)>>18));
			ret.nwrite = 4;
		}
		return ret;
	}
	Text::Code Text::UTF8To16(char32_t src) {
		Code ret = UTF8To32(src);
		return UTF32To16(ret.code);
	}
	// 変換後の数値, 読み取ったバイト数, 書き込んだバイト数
	Text::Code Text::UTF16To8(char32_t src) {
		Code ret = UTF16To32(src);
		return UTF32To8(ret.code);
	}

	//! 32bitデータの内、(1〜4)までの任意のバイト数をpDstに書き込む
	void Text::WriteData(void* pDst, char32_t val, int n) {
		switch(n) {
			case 1:
				*reinterpret_cast<uint8_t*>(pDst) = val&0xff; return;
			case 2:
				*reinterpret_cast<uint16_t*>(pDst) = val&0xffff; return;
			case 3:
				*reinterpret_cast<uint16_t*>(pDst) = val&0xffff;
				*reinterpret_cast<uint8_t*>(pDst) = (val>>16)&0xff; return;
			case 4:
				*reinterpret_cast<uint32_t*>(pDst) = val; return;
			default:
				assert(false);
		}
	}
	namespace {
		template <class DST, class SRC>
		std::basic_string<DST> ConvertToString(const std::vector<SRC>& data, const SRC* pCur) {
			auto* pSrcC = reinterpret_cast<const DST*>(&data[0]);
			return std::basic_string<DST>(pSrcC, reinterpret_cast<const DST*>(pCur) - pSrcC);
		}
		template <class DST, class SRC, class Conv>
		std::basic_string<DST> UTFConvert(const SRC* pSrc, size_t len, int ratio, Conv cnv) {
			std::vector<DST> ret(len*ratio);
			auto* pDst = &ret[0];
			auto* pSrcEnd = pSrc + len;
			while(pSrc != pSrcEnd) {
				Text::Code ret = cnv(*reinterpret_cast<const char32_t*>(pSrc));
				pSrc += ret.nread;
				Text::WriteData(pDst, ret.code, ret.nwrite * sizeof(decltype(*pDst)));
				pDst += ret.nwrite;
			}
			// 実際に使ったサイズに合わせる
			return ConvertToString<DST>(ret, pDst);
		}
	}
	std::string Text::UTFConvertTo8(c16Str src) {
		return UTFConvert<char>(src.getPtr(), src.getSize(), 2, &UTF16To8);
	}
	std::string Text::UTFConvertTo8(c32Str src) {
		return UTFConvert<char>(src.getPtr(), src.getSize(), 4, &UTF32To8);
	}
	std::u16string Text::UTFConvertTo16(c8Str src) {
		return UTFConvert<char16_t>(src.getPtr(), src.getSize(), 1, &UTF8To16);
	}
	std::u16string Text::UTFConvertTo16(c32Str src) {
		return UTFConvert<char16_t>(src.getPtr(), src.getSize(), 2, &UTF32To16);
	}
	std::u32string Text::UTFConvertTo32(c16Str src) {
		return UTFConvert<char32_t>(src.getPtr(), src.getSize(), 2, &UTF16To32);
	}
	std::u32string Text::UTFConvertTo32(c8Str src) {
		return UTFConvert<char32_t>(src.getPtr(), src.getSize(), 1, &UTF8To32);
	}
}
