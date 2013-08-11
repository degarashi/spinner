#pragma once
#include "bits.hpp"

namespace spn {
	//! レイヤー分離したビット配列
	template <int N, class T=uint32_t>
	class LayerBitArray {
		const static int ElemWidth = NBits<sizeof(T)*8-1>::result,
						ElemBits = sizeof(T)*8,
						NElemLow = (N+ElemBits-1)/ElemBits,			// 最下層のレイヤーでElemを幾つ使うか
						NElem = NDivide<NElemLow, ElemBits>::sum+1,
						NLayer = NDivide<NElemLow,ElemBits>::result+1;	// 最上位を1Elemとした時に幾つレイヤーが必要か

		template <int LayerFromLow, int Value>
		struct _OffsetInv {
			constexpr static int result = (Value+ElemBits-1)/ElemBits + _OffsetInv<LayerFromLow-1, Value/ElemBits>::result;
		};
		template <int Value>
		struct _OffsetInv<0, Value> {
			constexpr static int result = (Value+ElemBits-1)/ElemBits;
		};
		template <int Layer>
		struct _Offset {
			constexpr static int result = NElem - _OffsetInv<NLayer-Layer, N>::result;
		};
		// 各レイヤーのオフセット
		template <int Layer>
		using Offset = _Offset<Layer>;

		// 一次元配列で扱う
		T	_bit[NElem];
		// 総フラグ数
		int _nFlag;
		// NはBitsの整数倍でなければならない
		int _dummy[N%ElemBits == 0 ? 0 : -1];

		template <int Layer, int EndLayer, class Proc>
		struct Iter {
			static void proc(const LayerBitArray& src, Proc& p, int cur) {
				// フラグをシフトしながら巡回
				T bs = src._bit[Offset<Layer>::result + cur];

				int bsLsb = Bit::LSB_N(bs);
				bs >>= bsLsb;
				int inCur = cur*ElemBits+bsLsb;
				while(bs != 0) {
					if(bs & 1)
						Iter< Layer+1, EndLayer, Proc>::proc(src, p, inCur);
					++inCur;
					bs >>= 1;
				}
			}
		};

		template <int Layer, class Proc>
		struct Iter<Layer, Layer, Proc> {
			static void proc(const LayerBitArray& src, Proc& p, int cur) {
				T bs = src._bit[Offset<Layer>::result + cur];

				int bsLsb = Bit::LSB_N(bs);
				bs >>= bsLsb;
				int inCur = cur*ElemBits+bsLsb;
				while(bs != 0) {
					if(bs & 1)
						p(inCur);
					++inCur;
					bs >>= 1;
				}
			}
		};
		struct Setter {
			static void proc(T& val, const T mask, const T /*prev*/) { val |= mask; }
			static void count(int& cnt, const T e, const T mask) { cnt += ~Bit::ZeroOrFull(e & mask) & 1; }
		};
		struct Clearer {
			static void proc(T& val, const T mask, const T prev) {
				if(prev == 0)
					val &= ~mask;
			}
			static void count(int& cnt, const T e, const T mask) { cnt -= ~Bit::ZeroOrFull(e & mask)+1; }
		};
		template <class VAL>
		struct Collector {
			std::vector<VAL> _lst;
			VAL*			_ptr;
			Collector(int n): _lst(n), _ptr(&_lst[0]) {}

			void operator()(int idx) { *_ptr++ = idx; }
		};
		struct Callback {
			std::function<void (int)>&	_cb;
			Callback(std::function<void (int)>& cb): _cb(cb) {}
			void operator()(int idx) { _cb(idx); }
		};
		template <class Proc>
		void _iterate(Proc& p) const {
			Iter<0,NLayer,Proc>::proc(*this,p,0);
		}
		template <class Proc>
		void _proc(int idx) {
			constexpr uint32_t ofsMask = (1 << ElemWidth) - 1;
			uint32_t ofs = idx;

			T mask = 1<<(ofs & ofsMask);
			T* plCur = _bit + Offset<NLayer>::result;
			T* prev = plCur + (ofs>>ElemWidth);
			Proc::count(_nFlag, *prev, mask);
			Proc::proc(*prev, mask, 0);

			ofs >>= ElemWidth;
			// Elemの特定と上レイヤの巡回
			int lwidth = (NElemLow+ElemBits-1) >> ElemWidth;
			plCur -= lwidth;
			for(int i=NLayer-1 ; i>=0 ; i--) {
				int nofs = ofs >> ElemWidth;
				const T mask = 1<<(ofs & ofsMask);
				T* tCur = plCur + nofs;
				Proc::proc(*tCur, mask, *prev);
				ofs = nofs;
				prev = tCur;

				lwidth = (lwidth+ElemBits-1) >> ElemWidth;
				plCur -= lwidth;
			}
		}
		public:
			LayerBitArray() {
				clear();
			}
			//! ビットを全て0にする
			void clear() {
				memset(this, 0, sizeof(*this));
			}

			template <class OP>
			void _procIndex() {}
			template <class OP, class... Idx>
			void _procIndex(int idx0, Idx... idx) {
				_proc<OP>(idx0);
				_procIndex<OP>(idx...);
			}

			//! 指定ビットを1にする
			template <class... Idx>
			void set(Idx... idx) {
				_procIndex<Setter>(idx...);
			}
			//! 指定ビットを0にする
			template <class... Idx>
			void reset(Idx... idx) {
				_procIndex<Clearer>(idx...);
			}
			//! 指定ビットが立っているかをチェック
			bool check(int idx) const {
				return _bit[Offset<NLayer>::result + (idx>>ElemWidth)] & (1 << (idx>>ElemWidth));
			}
			//! 1になっているビットの数
			int getNBit() const { return _nFlag; }

			//! 1になっているビットのインデックス配列を取得
			template <class VAL>
			std::vector<VAL> getIndexList() const {
				int nB = getNBit();
				if(nB > 0) {
					Collector<VAL> c(nB);
					_iterate(c);
					return std::move(c._lst);
				}
				return std::vector<VAL>();
			}
			//! 1になっているビットをコールバック関数で巡回
			void iterateBit(std::function<void (int)> cb) const {
				Callback c(cb);
				_iterate(c);
			}

			template <class ARDST, class AR0, class AR1, class OP>
			static ARDST& _operateOr (ARDST& arDst, const AR0& ar0, const AR1& ar1, OP op) {
				constexpr int low = Offset<NLayer>::result;
				// 上層
				for(int i=0 ; i<low ; i++)
					arDst._bit[i] = op(ar0._bit[i], ar1._bit[i]);

				// 最下層
				// ついでに総フラグ数の再カウント
				int count = 0;
				for(int i=0 ; i<NElem ; i++) {
					arDst._bit[i] = op(ar0._bit[i], ar1._bit[i]);
					count += Bit::Count(arDst._bit[i]);
				}
				arDst._nFlag = count;
				return arDst;
			}
			template <class ARDST, class AR0, class AR1, class OP>
			static ARDST& _operateAndXor(ARDST& arDst, const AR0& ar0, const AR1& ar1, OP op) {
				// 最下層のビット演算
				int count = 0;
				for(int i=Offset<NLayer>::result ; i<countof(_bit) ; i++) {
					arDst._bit[i] = op(ar0._bit[i], ar1._bit[i]);
					count += Bit::Count(arDst._bit[i]);
				}
				arDst._nFlag = count;
				// 上層は計算しなおし
				arDst._reconstructUpper();
				return arDst;
			}
			void _reconstructUpper() {
				int beg = Offset<NLayer>::result,
					end = countof(_bit);
				int upBeg = Offset<NLayer-1>::result,
					upEnd = beg;
				int lwidth = (NElemLow+ElemBits-1) >> ElemWidth;

				std::memset(_bit, 0, sizeof(T)*Offset<NLayer>::result);

				for(;;) {
					uint32_t bitmask = 0x01;
					int tmpUp = upBeg;
					for(int i=beg ; i<end ; i++) {
						_bit[tmpUp] |= Bit::ZeroOrFull(_bit[i]) & bitmask;
						bitmask <<= 1;
						uint32_t ovf = ~Bit::ZeroOrFull(bitmask) & 0x01;
						bitmask |= ovf;
						tmpUp += ovf;
					}
					assert(tmpUp >= upEnd-1);

					if(upBeg == 0)
						break;
					end = beg;
					beg = upBeg;
					upEnd = upBeg;
					lwidth = (lwidth+ElemBits-1) >> ElemWidth;
					upBeg -= lwidth;
				}
			}

			#define FUNC_OR [](const T& v0, const T& v1) { return v0 | v1; }
			#define FUNC_AND [](const T& v0, const T& v1) { return v0 & v1; }
			#define FUNC_XOR [](const T& v0, const T& v1) { return v0 ^ v1; }

			//! LayerBit同士をOR演算
			LayerBitArray& operator |= (const LayerBitArray& ar) {
				return _operateOr(*this, *this, ar, FUNC_OR); }
			LayerBitArray operator | (const LayerBitArray& ar) const {
				return _operateOr(LayerBitArray(), *this, ar, FUNC_OR); }
			//! LayerBit同士をAND演算
			LayerBitArray& operator &= (const LayerBitArray& ar) {
				return _operateAndXor(*this, *this, ar, FUNC_AND); }
			LayerBitArray operator & (const LayerBitArray& ar) const {
				return _operateAndXor(LayerBitArray(), *this, ar, FUNC_AND); }
			//! LayerBit同士をXOR演算
			LayerBitArray& operator ^= (const LayerBitArray& ar) {
				return _operateAndXor(*this, *this, ar, FUNC_XOR); }
			LayerBitArray operator ^ (const LayerBitArray& ar) const {
				return _operateAndXor(LayerBitArray(), *this, ar, FUNC_XOR); }

			#undef FUNC_OR
			#undef FUNC_AND
			#undef FUNC_XOR

			bool operator == (const LayerBitArray& ar) const {
				// ビット数が違っていたらアウト
				if(getNBit() != ar.getNBit())
					return false;

				// 一旦上層レイヤーで比較した方が速いかもしれない
				T val(0);

				for(int cur = Offset<NLayer>::result ; cur != countof(_bit) ; cur++)
					val |= _bit[cur] ^ ar._bit[cur];
				return val == 0;
			}
			bool operator != (const LayerBitArray& ar) const {
				return !(operator ==(ar));
			}
	};
}
