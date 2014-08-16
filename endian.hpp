#pragma once
#include <cstdint>

namespace spn {
	#define MoveByte(val,from,to) (((val>>from)&0xff) << to)
	#define FlipByte(val, pos) MoveByte(val, pos, (numbit-pos-8))
	inline uint64_t FlipEndian(uint64_t val) {
		constexpr int numbit = sizeof(val)*8;
		return FlipByte(val, 0) | FlipByte(val, 8) | FlipByte(val, 16) | FlipByte(val, 24)
				| FlipByte(val, 32) | FlipByte(val, 40) | FlipByte(val, 48) | FlipByte(val, 56);
	}
	inline uint32_t FlipEndian(uint32_t val) {
		constexpr int numbit = sizeof(val)*8;
		return FlipByte(val, 0) | FlipByte(val, 8) | FlipByte(val, 16) | FlipByte(val, 24);
	}
	inline uint16_t FlipEndian(uint16_t val) {
		constexpr int numbit = sizeof(val)*8;
		return FlipByte(val, 0) | FlipByte(val, 8);
	}
}

