#pragma once
#include <cstdint>

namespace spn {
	//! 32bitの4文字チャンクを生成
	#define MAKECHUNK(c0,c1,c2,c3) ((c3<<24) | (c2<<16) | (c1<<8) | c0)
	//! 4つの8bit値から32bitチャンクを生成
	inline static long MakeChunk(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3) {
		return (c3 << 24) | (c2 << 16) | (c1 << 8) | c0; }

	inline uint64_t U64FromU32(uint32_t hv, uint32_t lv) {
		uint64_t ret(hv);
		ret <<= 32;
		ret |= lv;
		return ret;
	}
}

