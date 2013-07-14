#pragma once
#include <cstdint>
#include <limits>
#include "type.hpp"

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

	//! ビットフィールド定義用
	template <int OFS, int LEN>
	struct BitF {
		enum { offset=OFS,
		length=LEN };
	};
	//! 特定のビットを対象として値を取得 & 代入
	/*! \param T 値を保持する型 (unsigned)
	 * 	\param OFS ビットオフセット
	 * 	\param LEN ビット長 */
	template <class T, int OFS, int LEN>
	class BitOp {
		enum { offset=OFS,
		length=LEN };
		T&	_value;
		constexpr static T MASKLOW = ((1 << LEN) - 1),
		MASK =  MASKLOW << OFS;
		// Tが非数値型だったりsignedな型ならコンパイルエラーを起こす
		using Limits = std::numeric_limits<T>;
		constexpr static int Dummy[Limits::is_specialized && Limits::is_integer && !Limits::is_signed ? 0 : -1] = {};

	public:
		template <class P>
		BitOp(P* ptr): _value(*(T*)ptr) {}
		BitOp(T* ptr): _value(*ptr) {}
		BitOp(T& ref): _value(ref) {}

		template <class V>
		BitOp& operator = (const V v) {
			// ビットクリア
			_value &= ~MASK;
			_value |= (T(v)<<OFS & MASK);
			return *this;
		}

		operator T () const {
			return (_value >> OFS) & MASKLOW;
		}
	};
	//! ワードに対するビット操作
	/*! \param WORD 値を保持する型
	 * 	\param OFS 下位ビット位置
	 * 	\param LEN 処理対象ビット幅 */
	template <class WORD, int OFS, int LEN>
	class BitSub {
		const static WORD MASKLOW = ((1 << LEN) - 1),
		MASK =  MASKLOW << OFS;
		using Word = WORD;
		Word&	_word;

	public:
		BitSub(Word& w): _word(w) {}

		template <class V>
		BitSub& operator = (const V v) {
			// ビットクリア
			_word &= ~MASK;
			_word |= (Word(v)<<OFS & MASK);
			return *this;
		}
		operator Word() const {
			return get(_word);
		}
		static Word get(const Word& w) {
			return (w >> OFS) & MASKLOW;
		}
		//! 違う基本型やビット位置でも交換可能だが、サイズ違いは駄目
		template <class T_WORD, int T_OFS>
		void swap(BitSub<T_WORD, T_OFS, LEN>& bs) noexcept {
			Word val0(*this);
			(*this) = Word(bs);
			bs = val0;
		}
	};
	//! テンプレートにてビットフィールドを定義
	/*! \param T 値を保持する型
	 \*param Args ビットフィールドを定義するBitOpクラス */
	template <class T, class... Ts>
	struct BitDef : CType<Ts...> {
		// Tが非数値型だったりsignedな型ならコンパイルエラーを起こす
		using Limits = std::numeric_limits<T>;
		constexpr static int UnsignedCheck[Limits::is_specialized && Limits::is_integer && !Limits::is_signed ? 0 : -1] = {};

		using Word = T;
	};
	//! ビットフィールドテンプレートクラス
	/*! \example
		struct MyDef : BitDef<uint32_t, BitF<0,14>, BitF<14,6>, BitF<20,12>> {	<br>
				enum { VAL0, VAL1, VAL2 }; };									<br>
		using Value = BitField<MyDef>;											<br>
		Value value;															<br>
		value.at<Value::VAL0>() = 128;											<br>
		int toGet = value.at<Value::VAL0>();

		\param BF BitDefクラス */
	template <class BF>
	struct BitField : BF {
		// BF(CTypes)から総サイズとオフセット計算
		constexpr static int maxbit = BF::template Size<>::maxbit,
							buffsize = ((maxbit-1)/8)+1;
		using Word = typename BF::Word;
		constexpr static int WordCheck[buffsize > sizeof(Word) ? -1 : 0] = {};

		Word _word;

		BitField() {}
		BitField(const Word& w): _word(w) {}
		Word& value() { return _word; }
		const Word& value() const { return _word; }

		template <int N>
		using BFAt = typename BF::template At<N>::type;
		template <int N>
		using BitSubT = BitSub<Word, BFAt<N>::offset, BFAt<N>::length>;

		//! ビット領域参照
		template <int N>
		BitSubT<N> at() {
			return BitSubT<N>(_word);
		}
		template <int N>
		Word at() const {
			return BitSubT<N>::get(_word);
		}
		//! ビット幅マスク取得
		template <int N>
		static Word length_mask() {
			return (Word(1) << BFAt<N>::length) - 1;
		}
		//! ビットマスク取得
		template <int N>
		static Word mask() {
			auto ret = length_mask<N>();
			return ret << BFAt<N>::offset;
		}
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
			const static char NLRZ_TABLE[33];
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
namespace std {
	template <class W0, int OFS0, class W1, int OFS1, int LEN>
	inline void swap(spn::BitSub<W0,OFS0,LEN>&& b0, spn::BitSub<W1,OFS1,LEN>&& b1) { b0.swap(b1); }
	template <class W0, int OFS0, class W1, int OFS1, int LEN>
	inline void swap(spn::BitSub<W0,OFS0,LEN>&& b0, spn::BitSub<W1,OFS1,LEN>& b1) { b0.swap(b1); }
	template <class W0, int OFS0, class W1, int OFS1, int LEN>
	inline void swap(spn::BitSub<W0,OFS0,LEN>& b0, spn::BitSub<W1,OFS1,LEN>&& b1) { b0.swap(b1); }
}
