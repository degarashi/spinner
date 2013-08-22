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
}
