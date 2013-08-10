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

		// 一次元配列で扱う
		T	_bit[NElem];
		// 各レイヤーのオフセット
		int	_offset[NLayer+1];
		// 総フラグ数
		int _nFlag;
		// NはBitsの整数倍でなければならない
		int _dummy[N%ElemBits == 0 ? 0 : -1];

		template <int Layer, int EndLayer, class Proc>
		struct Iter {
			static void proc(const LayerBitArray& src, Proc& p, int cur) {
				// フラグをシフトしながら巡回
				T bs = src._bit[src._offset[Layer] + cur];

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
				T bs = src._bit[src._offset[Layer] + cur];

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
			int ofs = idx;
			T mask = 1<<(ofs % ElemBits);
			auto& e = _bit[_offset[NLayer] + ofs/ElemBits];
			Proc::count(_nFlag, e, mask);
			Proc::proc(e, mask, 0);
			auto prev = e;

			ofs >>= ElemWidth;
			// Elemの特定と上レイヤの巡回
			for(int i=NLayer-1 ; i>=0 ; i--) {
				int nofs = ofs >> ElemWidth;
				const T mask = 1<<(ofs % ElemBits);
				Proc::proc(_bit[_offset[i] + nofs], mask, prev);
				prev = _bit[_offset[i] + nofs];
				ofs = nofs;
			}
		}
		public:
			LayerBitArray() {
				// TODO: この処理は静的にできる筈
				int cur = NElem,
					sub = NElemLow;
				for(int i=NLayer ; i>=0 ; i--) {
					cur -= sub;
					sub /= ElemBits;
					_offset[i] = cur;
				}
				_offset[0] = 0;
				clear();
			}
			//! ビットを全て0にする
			void clear() {
				_nFlag = 0;
				memset(_bit, 0, sizeof(_bit));
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
				return _bit[_offset[NLayer] + idx/ElemBits] & (1<<(idx % ElemBits));
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
			static ARDST& _operate (ARDST& arDst, const AR0& ar0, const AR1& ar1, OP op) {
				const int low = ar0._offset[countof(_offset)-1];
				// 上層
				int cur = 0;
				while(cur != low) {
					arDst._bit[cur] = op(ar0._bit[cur], ar1._bit[cur]);
					++cur;
				}

				// 最下層
				// ついでに総フラグ数の再カウント
				int count = 0;
				while(cur != countof(ar0._bit)) {
					arDst._bit[cur] = op(ar0._bit[cur], ar1._bit[cur]);
					count += Bit::Count(arDst._bit[cur]);
					++cur;
				}
				arDst._nFlag = count;
				return arDst;
			}
			template <class OP>
			LayerBitArray& _operate2 (const LayerBitArray& ar, OP op) {
				return _operate(*this, *this, ar, op); }
			template <class OP>
			LayerBitArray _operate3 (const LayerBitArray& ar, OP op) const {
				LayerBitArray ret;
				std::memcpy(ret._offset, _offset, sizeof(_offset));
				return _operate(ret, *this, ar, op);
			}

			#define FUNC_OR [](const T& v0, const T& v1) { return v0 | v1; }
			#define FUNC_AND [](const T& v0, const T& v1) { return v0 & v1; }
			#define FUNC_XOR [](const T& v0, const T& v1) { return v0 ^ v1; }

			//! LayerBit同士をOR演算
			LayerBitArray& operator |= (const LayerBitArray& ar) {
				return _operate2(ar, FUNC_OR); }
			LayerBitArray operator | (const LayerBitArray& ar) const {
				return _operate3(ar, FUNC_OR); }
			//! LayerBit同士をAND演算
			LayerBitArray& operator &= (const LayerBitArray& ar) {
				return _operate2(ar, FUNC_AND); }
			LayerBitArray operator & (const LayerBitArray& ar) const {
				return _operate3(ar, FUNC_AND); }
			//! LayerBit同士をXOR演算
			LayerBitArray& operator ^= (const LayerBitArray& ar) {
				return _operate2(ar, FUNC_XOR); }
			LayerBitArray operator ^ (const LayerBitArray& ar) const {
				return _operate3(ar, FUNC_XOR); }

			#undef FUNC_OR
			#undef FUNC_AND
			#undef FUNC_XOR

			bool operator == (const LayerBitArray& ar) const {
				// ビット数が違っていたらアウト
				if(getNBit() != ar.getNBit())
					return false;

				// 一旦上層レイヤーで比較した方が速いかもしれない
				T val(0);
				for(int cur = _offset[countof(_offset)-1] ; cur != countof(_bit) ; cur++)
					val |= _bit[cur] ^ ar._bit[cur];
				return val == 0;
			}
			bool operator != (const LayerBitArray& ar) const {
				return !(operator ==(ar));
			}
	};
}
