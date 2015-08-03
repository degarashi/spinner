#pragma once
#include <type_traits>
#include <stack>
#include <deque>
#include "error.hpp"

namespace spn {
	//! 配列などのインデックスをフリーリストで管理
	template <class T, class= typename std::enable_if<std::is_integral<T>::value>::type>
	class FreeList {
		private:
			using IDStk = std::stack<T,std::deque<T>>;
			T		_maxV, _curV;
			IDStk	_stk;
		public:
			FreeList(T maxV, T startV): _maxV(maxV), _curV(startV) {}
			FreeList(FreeList&& fl): _maxV(fl._maxV), _curV(fl._curV), _stk(fl._stk) {}

			T get() {
				if(_stk.empty()) {
					T ret = _curV++;
					Assert(Trap, _curV != _maxV)
					return ret;
				}
				T val = _stk.top();
				_stk.pop();
				return val;
			}
			void put(T val) {
				_stk.push(val);
			}
			void swap(FreeList& f) noexcept {
				std::swap(_maxV, f._maxV);
				std::swap(_curV, f._curV);
				std::swap(_stk, f._stk);
			}
	};
	//! 複数のオブジェクトを単一バッファのフリーリストで管理
	/*! \param bExpand */
	template <class T, bool bExpand>
	class FreeObj {
		public:
			using Buffer = std::vector<uint8_t>;
			using ObjStack = std::vector<int>;

			// 一般のポインタと混同させないためにラップする
			class Ptr {
				friend class FreeObj<T, bExpand>;
				FreeObj*	_fobj;
				int			_idx;
				public:
					Ptr() = default;
					Ptr(FreeObj& fo, int idx): _fobj(&fo), _idx(idx) {}
					T* get() { return _fobj->_getBuff(_idx); }
					const T* get() const { return _fobj->_getBuff(_idx); }
					T* operator -> () { return get(); }
					const T* operator ->() const { return get(); }
			};

		private:
			int			_nCur;
			Buffer		_buff;
			ObjStack	_freeIdx;
			T* _getBuff(int idx) {
				return reinterpret_cast<T*>(&_buff[0]) + idx; }
			const T* _getBuff(int idx) const {
				return reinterpret_cast<const T*>(&_buff[0]) + idx; }

		public:
			FreeObj(int n): _nCur(0), _buff(n*sizeof(T)) {
				Assert(Trap, n > 0, "invalid object count") }
			FreeObj(const FreeObj&) = delete;
			FreeObj(FreeObj&& fo): _nCur(fo._nCur), _buff(std::move(fo._buff)), _freeIdx(std::move(fo._freeIdx)) {}
			~FreeObj() {
				clear();
			}

			int getNextID() const {
				if(_freeIdx.empty())
					return _nCur;
				return _freeIdx.back();
			}
			template <class... Args>
			Ptr get(Args&&... args) {
				// フリーオブジェクトを特定してコンストラクタを呼んで返す
				int idx;
				if(_freeIdx.empty()) {
					idx = _nCur++;
					if(_nCur > static_cast<int>(_buff.size()/sizeof(T))) {
						Assert(Trap, bExpand, "no more free object");
						_buff.resize(_buff.size() + (_buff.size() >> 1));
					}
				} else {
					idx = _freeIdx.back();
					_freeIdx.pop_back();
				}
				new (_getBuff(idx)) T(std::forward<Args>(args)...);
				return Ptr(*this, idx);
			}
			void put(const Ptr& ptr) {
				// デストラクタを呼んでインデックスをフリーリストに積む
				ptr.get()->~T();
				_freeIdx.push_back(ptr._idx);
			}
			void clear() {
				int nC = _nCur;
				_nCur = 0;
				for(int i=0 ; i<nC ; i++) {
					if(std::find(_freeIdx.begin(), _freeIdx.end(), i) == _freeIdx.end())
						_getBuff(i)->~T();
				}
				_freeIdx.clear();
			}
	};
}

