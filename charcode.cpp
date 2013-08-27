#include "misc.hpp"
#include <locale>
namespace spn {
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

	bool Text::utf16_isSurrogate(char16_t c) {
		return (c & 0xdc00) == 0xd800;
	}
	size_t Text::utf16_strlen(const char16_t* str) {
		int cur = 0;
		size_t count = 0;
		char16_t c = str[cur++];
		while(c != L'\0') {
			++count;
			if(Text::utf16_isSurrogate(c))
				++cur;
			c = str[cur++];
		}
		return count;
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

	void Text::UTF16To32(uint32_t src, uint32_t& code, int& nread) {
		if((src & 0xdc00dc00) == 0xdc00d800) {
			// サロゲートペア
			uint32_t l_bit = src & 0x03ff0000,
			h_bit = (src & 0x000003ff) + 0x40;

			code = (l_bit>>16)|(h_bit<<10);
			nread = 4;
		} else {
			// 16bit数値
			code = src&0xffff;
			nread = 2;
		}
	}
	void Text::UTF32To16(uint32_t src, uint32_t& code, int& nread) {
		if(src >= 0x10000) {
			// サロゲートペア
			uint32_t l_bit = (src & 0x3ff)<<16,
			h_bit = ((src>>10)&0x3ff) - 0x40;
			code = h_bit|l_bit|0xdc00d800;
			nread = 4;
		} else {
			// 16bit数値
			code = src;
			nread = 2;
		}
	}
	void Text::UTF8To32(uint32_t src, uint32_t& code, int& nread) {
		if((src&0x80) == 0) {
			// 1バイト文字
			code = src&0xff;
			nread = 1;
		} else if((src&0xc0e0) == 0x80c0) {
			// 2バイト文字
			code = ((src&0x1f)<<6) |
					((src&0x3f00)>>8);
			nread = 2;
		} else if((src&0xc0c0f0) == 0x8080e0) {
			// 3バイト文字
			code = ((src&0x0f)<<12) |
					((src&0x3f00)>>2) |
					((src&0x3f0000)>>16);
			nread = 3;
		} else if((src&0xc0c0c0f8) == 0x808080f0) {
			// 4バイト文字
			code = ((src&0x07)<<18) |
					((src&0x3f00)<<4) |
					((src&0x3f0000)>>10) |
					((src&0x3f000000)>>24);
			nread = 4;
		}
		throw std::invalid_argument("unknown unicode char");
	}
	// 不正なシーケンスを検出すると例外を発生させる
	void Text::UTF8To32_s(uint32_t src, uint32_t& code, int& nread) {
		uint32_t val;
		int nByte;
		UTF8To32(src, val, nByte);
		const uint32_t mask[5] = {0, 0,0x1e00,0x0f2000,0x07300000};
		const uint32_t ormask[5] = {0, 1,0,0,0};

		if((val & mask[nByte]) | ormask[nByte]) {
			code = val;
			nread = nByte;
		} else
			throw std::invalid_argument("invalid unicode sequence");
	}
	// 変換後の数値, 書き込んだバイト数
	void Text::UTF32To8(uint32_t src, uint32_t& code, int& nread) {
		if(src < 0x80) {
			// 1バイト文字
			code = src;
			nread = 1;
		} else if(src < 0x0400) {
			// 2バイト文字
			code = ((0x80 | (src&0x3f))<<8) |
					(0xc0 | ((src&0x07c0)>>6));
			nread = 2;
		} else if(src < 0x10000) {
			// 3バイト文字
			code = ((0x80 | (src&0x3f))<<16) |
					((0x80 | ((src&0x0fc0)>>6))<<8) |
					(0xe0 | ((src&0xf000)>>12));
			nread = 3;
		} else {
			// 4バイト文字
			code = ((0x80 | (src&0x3f))<<24) |
					((0x80 | ((src&0x0fc0)>>6))<<16) |
					((0x80 | ((src&0x3f000)>>12))<<8) |
					(0xf0 | ((src&0x1c0000)>>18));
			nread = 4;
		}
	}
	void Text::UTF8To16(uint32_t src, uint32_t& code, int& nread, int& nwrite) {
		uint32_t val;
		UTF8To32(src, val, nread);
		UTF32To16(val, code, nwrite);
	}
	// 変換後の数値, 読み取ったバイト数, 書き込んだバイト数
	void Text::UTF16To8(uint32_t src, uint32_t& code, int& nread, int& nwrite) {
		uint32_t val;
		UTF16To32(src, val, nread);
		UTF32To8(val, code, nwrite);
	}
	int Text::UTFConvert8_16(char16_t* dst, size_t n_dst, const char* src, size_t n_src) {
		if(n_src == 0)
			n_src = std::strlen(src);
		// 少なくとも同じ文字数でなければ収まらない
		if(n_dst < n_src)
			return -1;

		char16_t* tdst = dst;
		uint32_t val;
		size_t count = 0;
		int nRead, nWrite;
		while(count < n_src) {
			UTF8To16(*((uint32_t*)src), val, nRead, nWrite);
			src += nRead;
			count += nRead;
			// 出力文字数に応じて書き込む
			// 通常=2, サロゲート=4
			const char16_t* wc = (const char16_t*)&val;
			do {
				*tdst++ = *wc++;
				nWrite -= 2;
			} while(nWrite != 0);
		}
		return tdst-dst;
	}
	int Text::UTFConvert16_8(char* dst, size_t n_dst, const char16_t* src, size_t n_src) {
		if(n_src == 0)
			n_src = utf16_strlen(src);
		if(n_dst < n_src)
			return -1;

		char* tdst = dst;
		uint32_t val;
		size_t count = 0;
		int nRead, nWrite;
		while(count < n_src) {
			UTF16To8(*((uint32_t*)src), val, nRead, nWrite);
			src += nRead/2;
			count += nRead/2;

			// nWrite(1～4)バイト可変
			const char* c = (const char*)&val;
			do {
				*tdst++ = *c++;
			} while(--nWrite != 0);
		}
		return tdst-dst;
	}
}