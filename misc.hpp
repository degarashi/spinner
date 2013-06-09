#pragma once
#include <cassert>
#include <cstring>
#include <cwchar>
#include <stack>
#include <deque>
#include <atomic>
#include <thread>

//! 32bitの4文字チャンクを生成
#define MAKECHUNK(c0,c1,c2,c3) ((c3<<24) | (c2<<16) | (c1<<8) | c0)

namespace spn {
	//! スピンロックによるmutex
	class Synchro {
		private:
			using AThID = std::atomic<std::thread::id>;

			std::atomic_bool	_abLock;	//!< 誰かロックしてるか
			AThID				_idTh;		//!< ロック中のスレッド番号
			int					_iLockNum;	//!< 再帰的なロック用カウンタ

			template <bool Block>
			bool _lock() {
				auto thid = std::this_thread::get_id();
				do {
					bool b = false;
					if(_abLock.compare_exchange_weak(b, true, std::memory_order_acquire)) {
						_iLockNum = 1;
						_idTh.store(thid, std::memory_order_relaxed);
						return true;
					}
					if(_idTh.compare_exchange_weak(thid, thid, std::memory_order_acquire)) {
						++_iLockNum;
						return true;
					}
				} while(Block);
				return false;
			}
		public:
			Synchro(): _abLock(false) {}

			void lock() { _lock<true>(); }
			bool try_lock() { return _lock<false>(); }
			void unlock() {
				if(--_iLockNum == 0)
					_abLock.store(false, std::memory_order_relaxed);
			}
	};

	//! 何もしないダミーmutex
	class PseudoSynch {
		public:
			void lock() {}
			bool try_lock() { return true; }
			void unlock() {}
	};

	//! 4つの8bit値から32bitチャンクを生成
	inline static long MakeChunk(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3) {
		return (c3 << 24) | (c2 << 16) | (c1 << 8) | c0; }

	inline uint64_t U64FromU32(uint32_t hv, uint32_t lv) {
		uint64_t ret(hv);
		ret <<= 32;
		ret |= lv;
		return ret;
	}

	struct Text {
		// エンコードタイプ
		enum class CODETYPE {
			UTF_8,
			UTF_16LE,
			UTF_16BE,
			UTF_32LE,
			UTF_32BE,
			ANSI
		};
		//! SJISコードか判定する
		static bool sjis_isMBChar(char c);
		//! SJISコードにおける文字数判定
		static int sjis_strlen(const char* str);
		//! エンコードタイプと判定に使った文字数を返す
		static std::pair<CODETYPE, int> GetEncodeType(const void* ptr);
		//! 文字列の先頭4文字から32bitチャンクを生成
		inline static long MakeChunk(const char* str) {
			return ::spn::MakeChunk(str[0], str[1], str[2], str[3]); }
	};

	//! フリーリストでオブジェクト管理
	template <class T>
	class FreeList {
		private:
			T		_maxV, _curV;
			std::stack<T,std::deque<T>>	_stk;
		public:
			FreeList(T maxV, T startV): _maxV(maxV), _curV(startV) {}
			FreeList(FreeList&& fl): _maxV(fl._maxV), _curV(fl._curV), _stk(fl._stk) {}

			T get() {
				if(_stk.empty()) {
					int ret = _curV++;
					assert(_curV != _maxV);
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
	//! ポインタ読み替え変換
	template <class T0, class T1>
	inline T1 RePret(const T0& val) {
		static_assert(sizeof(T0) == sizeof(T1), "invalid reinterpret value");
		return *reinterpret_cast<const T1*>(&val);
	}

	//! 値飽和
	template <class T>
	T Saturate(const T val, const T minV, const T maxV) {
		if(val > maxV)
			return maxV;
		if(val < minV)
			return minV;
		return val;
	}
	template <class T>
	T Saturate(const T val, const T range) {
		return Saturate(val, -range, range);
	}

	//! 値補間
	template <class T>
	T Lerp(const T v0, const T v1, float r) {
		return (v1-v0)*r + v0;
	}

	//! 汎用シングルトン
	template <typename T>
	class Singleton {
		private:
			static T* ms_singleton;
		public:
			Singleton() {
				assert(!ms_singleton || !"initializing error - already initialized");
				int offset = (int)(T*)1 - (int)(Singleton<T>*)(T*)1;
				ms_singleton = (T*)((int)this + offset);
			}
			virtual ~Singleton() {
				assert(ms_singleton || !"destructor error");
				ms_singleton = 0;
			}
			static T& _ref() {
				assert(ms_singleton || !"reference error");
				return *ms_singleton;
			}
	};
	template <typename T>
	T* Singleton<T>::ms_singleton = nullptr;

	//! 固定長文字列
	template <class CT>
	struct FixStrF;
	template <>
	struct FixStrF<char> {
		static void copy(char* dst, int n, const char* src) {
			std::strncpy(dst, src, n);
		}
		static int cmp(const char* v0, const char* v1) {
			return std::strcmp(v0, v1);
		}
	};
	template <>
	struct FixStrF<wchar_t> {
		static void copy(wchar_t* dst, int n, const wchar_t* src) {
			std::wcsncpy(dst, src, n);
		}
		static int cmp(const wchar_t* v0, const wchar_t* v1) {
			return std::wcscmp(v0, v1);
		}
	};
	template <class CT, int N>
	struct FixStr {
		typedef FixStrF<CT> FS;
		typedef CT value_type;
		enum {length=N};
		CT	str[N];

		FixStr(const CT* src=(const CT*)L"") {
			FS::copy(str, N, src);
		}
		FixStr(const CT* src_b, const CT* src_e) {
			int nC = src_e-src_b;
			assert(nC <= sizeof(CT)*(N-1));
			std::memcpy(str, src_b, nC);
			str[nC] = '\0';
		}
		operator const CT* () const {
			return str;
		}
		bool operator < (const CT* s) const {
			return FS::cmp(str, s) < 0;
		}
		bool operator == (const CT* s) const {
			return !FS::cmp(str, s);
		}
		bool operator != (const CT* s) const {
			return !(*this == s);
		}
		FixStr& operator = (const FixStr& s) {
			FS::copy(str, N, s);
			return *this;
		}
		CT& operator [] (int n) {
			return str[n];
		}
		bool operator < (const FixStr& s) const {
			return FS::cmp(str, s) < 0;
		}
		bool operator == (const FixStr& s) const {
			return !FS::cmp(str, s);
		}
		bool operator != (const FixStr& s) const {
			return !(*this == s);
		}
		bool empty() const {
			return str[0] == '\0';
		}
		void clear() {
			str[0] = '\0';
		}
	};
}
