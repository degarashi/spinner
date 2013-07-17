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
			//! 指定ビットを1にする
			void set(int idx) {
				_proc<Setter>(idx);
			}
			//! 指定ビットを0にする
			void reset(int idx) {
				_proc<Clearer>(idx);
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
	};
}
