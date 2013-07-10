#pragma once
#include "vector.hpp"
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
	T Saturate(const T& val, const T& minV, const T& maxV) {
		if(val > maxV)
			return maxV;
		if(val < minV)
			return minV;
		return val;
	}
	template <class T>
	T Saturate(const T& val, const T& range) {
		return Saturate(val, -range, range);
	}
	//! 値補間
	template <class T>
	T Lerp(const T& v0, const T& v1, float r) {
		return (v1-v0)*r + v0;
	}
	//! 値が範囲内に入っているか
	template <class T>
	bool IsInRange(const T& val, const T& vMin, const T& vMax) {
		return val>=vMin && val<=vMax;
	}
	template <class T>
	bool IsInRange(const T& val, const T& vMin, const T& vMax, const T& vEps) {
		return IsInRange(val, vMin-vEps, vMax+vEps);
	}
	//! aがb以上だったらaからsizeを引いた値を返す
	inline int CndSub(int a, int b, int size) {
		return a - (size & ((b - a - 1) >> 31));
	}
	//! aがb以上だったらaからbを引いた値を返す
	inline int CndSub(int a, int b) {
		return CndSub(a, b, b);
	}
	//! aがbより小さかったらaにsizeを足した値を返す
	inline int CndAdd(int a, int b, int size) {
		return a + (size & ((a - b) >> 31));
	}
	//! aが0より小さかったらaにsizeを足した値を返す
	inline int CndAdd(int a, int size) {
		return CndAdd(a, 0, size);
	}
	//! aがlowerより小さかったらsizeを足し、upper以上だったらsizeを引く
	inline int CndRange(int a, int lower, int upper, int size) {
		return a + (size & ((a - lower) >> 31))
					- (size & ((upper - a - 1) >> 31));
	}
	//! aが0より小さかったらsizeを足し、size以上だったらsizeを引く
	inline int CndRange(int a, int size) {
		return CndRange(a, 0, size, size);
	}
	//! ポインタの読み替え
	template <class T, class T2>
	T* ReinterpretPtr(T2* ptr) {
		using TD = typename std::decay<T2>::type;
		static_assert(std::is_integral<TD>::value || std::is_floating_point<TD>::value, "typename T must number");
		return reinterpret_cast<T*>(ptr);
	}
	//! リファレンスの読み替え
	template <class T, class T2>
	T& ReinterpretRef(T2& val) {
		return *reinterpret_cast<T*>(&val);
	}
	//! 値の読み替え
	template <class T, class T2>
	T ReinterpretValue(const T2& val) {
		return *reinterpret_cast<const T*>(&val);
	}
	constexpr uint32_t FloatOne = 0x3f800000;		//!< 1.0fの、整数表現
	//! 引数がプラスなら1, マイナスなら-1を返す
	float inline PlusMinus1(float val) {
		auto ival = spn::ReinterpretValue<uint32_t>(val);
		return spn::ReinterpretValue<float>(FloatOne | (ival & 0x80000000));
	}
	//! PlusMinus1(float)の引数がintバージョン
	float inline PlusMinus1(int val) {
		return spn::ReinterpretValue<float>(FloatOne | ((val>>31) & 0x80000000)); }
	//! PlusMinus1(float)の引数がunsigned intバージョン
	float inline PlusMinus1(unsigned int val) { return PlusMinus1(static_cast<int>(val)); }

	template <class T>
	T* AAllocBase(int nAlign, int n) {
		void* ptr;
		if(posix_memalign(&ptr, nAlign, sizeof(T)*n) != 0)
			throw std::bad_alloc();
		return reinterpret_cast<T*>(ptr);
	}
	//! バイトアラインメント付きのメモリ確保
	/*! 解放はdeleteで行う
		----------------------
		これだと解放の時に死ぬ為
		void* ptr = std::malloc(sizeof(T) + 15);
		ptr = (void*)(((uintptr_t)ptr + 15) & ~0x0f); */
	template <class T, class... Args>
	T* AAlloc(int n, Args... args) {
		return new(AAllocBase<T>(n,1)) T(std::forward<Args>(args)...);
	}
	//! デフォルトコンストラクタによる初期化
	template <class T>
	T* AAlloc(int n) {
		return new(AAllocBase<T>(n,1)) T();
	}
	//! initializer_listによる初期化
	template <class T, class A>
	T* AAlloc(int n, std::initializer_list<A>&& w) {
		return new(AAllocBase<T>(n,1)) T(std::forward<std::initializer_list<A>>(w));
	}
	//! バイトアラインメント付きの配列メモリ確保
	template <class T>
	T* AArray(int nAlign, int n) {
		T* ptr = AAllocBase<T>(nAlign, n);
		for(int i=0 ; i<n ; i++)
			new(ptr+i) T();
		return ptr;
	}
	//! アラインメントチェッカ
	template <int N, class T>
	class CheckAlign {
		protected:
			CheckAlign() {
				// 16byteアラインメントチェック
				assert((((uintptr_t)this)&(N-1)) == 0);
			}
		public:
			static T* New() {
				return AAlloc<T>(N);
			}
			template <class A>
			static T* New(std::initializer_list<A>&& w) {
				return AAlloc<T>(N, std::forward<std::initializer_list<A>>(w));
			}
			template <class... Args>
			static T* New(Args... args) {
				return AAlloc<T>(N, std::forward<Args>(args)...);
			}
	};

	//! dirAを基準に時計回りに増加する値を返す
	/*! \param[in] dir 値を算出したい単位ベクトル
		\param[in] dirA 基準の単位ベクトル
		\return 角度に応じた0〜4の値(一様ではない) */
	inline float AngleValue(const Vec2& dir, const Vec2& dirA) {
		float d0 = dir.dot(dirA);
		if(dirA.cw(dir) <= -1e-6f)
			return d0+1 + 2;
		return 2.f-(d0+1);
	}
	//! 上方向を基準としたdirの角度を返す
	inline float Angle(const Vec2& dir) {
		float ac0 = std::acos(dir.y);
		if(dir.x <= -1e-6f)
			return 2*spn::PI - ac0;
		return ac0;
	}
	//! 値が近いか
	/*! \param[in] val value to check
		\param[in] vExcept target value
		\param[in] vEps value threshold */
	template <class T>
	bool IsNear(const T& val, const T& vExcept, const T& vEps) {
		return IsInRange(val, vExcept-vEps, vExcept+vEps);
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
