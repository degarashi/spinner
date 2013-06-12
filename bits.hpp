#pragma once
#include <cstdint>

namespace spn {
	//! 表現に何ビット使うか計算
	/*! +1すればMSBの取得にもつかえる */
	template <unsigned int N>
	struct NBits {
		enum { result=1+NBits<((N)>>1)>::result };
	};
	template <>
	struct NBits<0> {
		enum { result=1 };
	};
	template <>
	struct NBits<1> {
		enum { result=1 };
	};

	//! NをLで何回割れるか(と、その余り)
	template <int N, int L, int N2=N>
	struct NDivide {
		typedef NDivide<N/L,L,N> Lower;
		enum { result=Lower::result + ((N>=L) ? 1 : 0),
				sum=N + Lower::sum,
				odd=Lower::odd };
	};
	template <int L, int N2>
	struct NDivide<0, L, N2> {
		enum { result=0,
				sum=0,
				odd=N2 };
	};

	//! コンパイル時累乗
	template <int N, int P>
	struct NPow {
		enum { result=NPow<N,P-1>::result * N };
	};
	template <int N>
	struct NPow<N,0> {
		enum { result=1 };
	};

	//! 引数のペアを全てXORしたものをORする
	inline int X_OrArgs() { return 0; }
	template <class T, class... Args>
	inline T X_OrArgs(const T& t0, const T& t1, Args... args) {
		return (t0^t1) | X_OrArgs(args...);
	}

	//! 定数Nの使用ビット
	template <int N>
	struct BitLength {
		enum { length = BitLength<(N>>1)>::length + 1 };
	};
	template <>
	struct BitLength<0> {
		enum { length = 0 };
	};

	struct Bit {
		static bool Check(uint32_t val, uint32_t flag) {
			return val & flag;
		}
		static void Clear(uint32_t& val, uint32_t flag) {
			val &= ~flag;
		}
		static void Set(uint32_t& val, uint32_t flag) {
			val |= flag;
		}
		static bool ChClear(uint32_t& val, uint32_t flag) {
			bool bRet = Check(val, flag);
			Clear(val, flag);
			return bRet;
		}
		static uint32_t LowClear(uint32_t x) {
			x = x | (x >> 1);
			x = x | (x >> 2);
			x = x | (x >> 4);
			x = x | (x >> 8);
			x = x | (x >>16);
			return x & ~(x>>1);
		}
		static int Count(uint32_t v) {
			uint32_t tmp = v & 0xaaaaaaaa;
			v &= 0x55555555;
			v = v + (tmp >> 1);
			tmp = v & 0xcccccccc;
			v &= 0x33333333;
			v = v + (tmp >> 2);
			tmp = v & 0xf0f0f0f0;
			v &= 0x0f0f0f0f;
			v = v + (tmp >> 4);
			tmp = v & 0xff00ff00;
			v &= 0x00ff00ff;
			v = v + (tmp >> 8);
			tmp = v & 0xffff0000;
			v &= 0x0000ffff;
			v = v + (tmp >> 16);
			return v;
		}
		static int32_t ZeroOrFull(int32_t v) {
			return (v | -v) >> 31;
		}

		// ビットの位置を計算
		#ifdef USEASM_BITSEARCH
			static uint32_t MSB_N(uint32_t x) {
				uint32_t ret;
				asm("movl %1, %%eax;"
					"orl $0x01, %%eax;"
					"bsrl %%eax, %0;"
					:"=r" (ret)
					:"r" (x)
					:"%eax"
				);
				return ret;
			}
			static uint32_t LSB_N(uint32_t x) {
				uint32_t ret;
				asm("movl %1, %%eax;"
					"orl $0x80000000, %%eax;"
					"bsfl %%eax, %0;"
					:"=r" (ret)
					:"r" (x)
					:"%eax"
				);
				return ret;
			}
		#else
			constexpr static char NLRZ_TABLE[] = "\x00\x00\x01\x0e\x1c\x02\x16\x0f\x1d\x1a\x03\x05\x0b\x17"
								"\x07\x10\x1e\x0d\x1b\x15\x19\x04\x0a\x06\x0c\x14\x18\x09"
								"\x13\x08\x12\x11";
			static uint32_t MSB_N(uint32_t x) {
				return NLRZ_TABLE[0x0aec7cd2U * LowClear(x) >> 27];
			}
			static uint32_t LSB_N(uint32_t x) {
				return NLRZ_TABLE[0x0aec7cd2U * (x & -x) >> 27];
			}
		#endif

		// エンディアン変換
		static uint32_t LtoB32(uint32_t v) {
			uint32_t a = v<<24,
				b = (v&0xff00)<<8,
				c = (v>>8)&0xff00,
				d = v>>24;
			return a|b|c|d;
		}
		static uint32_t LtoB16(uint32_t v) {
			uint32_t a = (v>>8)&0xff,
				b = (v&0xff)<<8;
			return a|b;
		}
	};
}
