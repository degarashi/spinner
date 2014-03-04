#pragma once
#include "vector.hpp"
#include "matrix.hpp"
#include "quat.hpp"
#include "bits.hpp"
#include "abstbuff.hpp"
#include "error.hpp"
#include "optional.hpp"
#include <cassert>
#include <cstring>
#include <cwchar>
#include <stack>
#include <deque>
#include <atomic>
#include <thread>
//! 32bitの4文字チャンクを生成
#define MAKECHUNK(c0,c1,c2,c3) ((c3<<24) | (c2<<16) | (c1<<8) | c0)
//! 特定のメソッドを持っているかチェック
/*! DEF_HASMETHOD(method)
	HasMethod_method<class_name>(nullptr) -> std::true_type or std::false_type */
#define DEF_HASMETHOD(method) \
	template <class T> \
	std::true_type HasMethod_##method(decltype(&T::method) = &T::method) { return std::true_type(); } \
	template <class T> \
	std::false_type HasMethod_##method(...) { return std::false_type(); }
//! 特定のオーバーロードされたメソッドを持っているかチェック
/*! DEF_HASMETHOD_OV(name, method)
	予めクラスにspn::none method(...); を持たせておく
	暗黙の変換も含まれる
	HasMethod_name<class_name>() -> std::true_type or std::false_type */
#define DEF_HASMETHOD_OV(name, method, ...) \
	DEF_CHECKMETHOD_OV(name, method) \
	template <class T> \
	decltype(CheckMethod_##name<T, __VA_ARGS__>()) HasMethod_##name() { return decltype(CheckMethod_##name<T, __VA_ARGS__>())(); }
#define DEF_CHECKMETHOD_OV(name, method) \
	template <class T, class... Args> \
	std::integral_constant<bool, !std::is_same<::spn::none_t, decltype(std::declval<T>().method(::spn::ReturnT<Args>()...))>::value> CheckMethod_##name() { \
		return std::integral_constant<bool, !std::is_same<::spn::none_t, decltype(std::declval<T>().method(::spn::ReturnT<Args>()...))>::value>(); }

//! 特定の型を持っているかチェック
/*! DEF_HASTYPE(type_name)
	HasType_type_name(nullptr) -> std::true_type or std::false_type */
#define DEF_HASTYPE(name) \
	template <class T> \
	std::true_type HasType_##name(typename T::name*) { return std::true_type(); } \
	template <class T> \
	std::false_type HasType_##name(...) { return std::false_type(); }

namespace spn {
	//! Y軸を上とした時のZベクトルに対するXベクトルを算出, またはベクトルに垂直なベクトルを得る
	template <bool A>
	VecT<3,A> GetVerticalVec(const VecT<3,A>& zvec) {
		using VT = VecT<3,A>;
		auto vt = zvec % VT(0,1,0);
		if(vt.len_sq() < 1e-6f)
			vt = zvec % VT(1,0,0);
		return vt.normalization();
	}
	template <bool A>
	spn::Optional<VecT<3,A>> NormalFromPoints(const VecT<3,A>& v0,const VecT<3,A>& v1, const VecT<3,A>& v2) {
		auto tmp = (v1 - v0) % (v2 - v0);
		if(tmp.len_sq() < 1e-6f)
			return spn::none;
		return tmp.normalization();
	}
	//! 任意の戻り値を返す関数 (定義のみ)
	template <class T>
	T ReturnT();
	//! 関数の戻り値型を取得
	/*! ReturnType<T>::type */
	template <class T>
	struct ReturnType;
	template <class RT, class... Args>
	struct ReturnType<RT (*)(Args...)> {
		using type = RT; };
	template <class RT, class OBJ, class... Args>
	struct ReturnType<RT (OBJ::*)(Args...)> {
		using type = RT; };
	template <class RT, class OBJ, class... Args>
	struct ReturnType<RT (OBJ::*)(Args...) const> {
		using type = RT; };

	//! 指定した順番で関数を呼び、クラスの初期化をする(マネージャの初期化用)
	/*! デストラクタは初期化の時と逆順で呼ばれる */
	template <class... Ts>
	struct MInitializer {
		MInitializer(bool) {}
		void release() {}
	};
	template <class T, class... Ts>
	struct MInitializer<T,Ts...> {
		bool		bTop;
		T			ptr;
		MInitializer<Ts...>	other;

		template <class TA, class... TsA>
		MInitializer(bool btop, TA&& t, TsA&&... tsa): bTop(btop), ptr(t()), other(false, std::forward<TsA>(tsa)...) {}
		~MInitializer() {
			if(bTop)
				release();
		}
		void release() {
			if(ptr) {
				other.release();
				delete ptr;
				ptr = nullptr;
			}
		}
	};
	//! MInitializerのヘルパ関数: newしたポインタを返すラムダ式を渡す
	/*! MInitializerF([](){ return Manager0(); },<br>
					[](){ return Another(); });<br>
		のように呼ぶ */
	template <class... Ts>
	auto MInitializerF(Ts&&... ts) -> MInitializer<decltype(std::declval<Ts>()())...> {
		return MInitializer<decltype(std::declval<Ts>()())...>(true, std::forward<Ts>(ts)...);
	}

	#ifdef UNIX
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
	#endif

	//! 何もしないダミーmutex
	class PseudoSynch {
		public:
			void lock() {}
			bool try_lock() { return true; }
			void unlock() {}
	};

	//! static変数の初期化を制御する
	template <class T>
	class NiftyCounterIdiom {
		static int s_niftyCounter;
		public:
			NiftyCounterIdiom() {
				if(s_niftyCounter++ == 0) {
					T::Initialize();
				}
			}
			~NiftyCounterIdiom() {
				if(--s_niftyCounter == 0) {
					T::Terminate();
				}
			}
	};
	template <class T>
	int NiftyCounterIdiom<T>::s_niftyCounter = 0;

	#define DEF_NIFTY_INITIALIZER(name)	static class name##_initializer : public spn::NiftyCounterIdiom<name> {} name##_initializer_obj;

	//! 4つの8bit値から32bitチャンクを生成
	inline static long MakeChunk(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3) {
		return (c3 << 24) | (c2 << 16) | (c1 << 8) | c0; }

	inline uint64_t U64FromU32(uint32_t hv, uint32_t lv) {
		uint64_t ret(hv);
		ret <<= 32;
		ret |= lv;
		return ret;
	}

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
	inline uint32_t ARGBtoRGBA(uint32_t val) {
		return (val & 0xff00ff00) | ((val>>16)&0xff) | ((val&0xff)<<16);
	}
	inline uint32_t SetAlphaR(uint32_t val) {
		return (val & 0x00ffffff) | ((val&0xff)<<24);
	}

	namespace Text {
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
		bool sjis_isMBChar(char c);
		//! SJISコードにおける文字数判定
		int sjis_strlen(const char* str);
		//! エンコードタイプと判定に使った文字数を返す
		std::pair<CODETYPE, int> GetEncodeType(const void* ptr);
		//! 文字列の先頭4文字から32bitチャンクを生成
		inline long MakeChunk(const char* str) {
			return ::spn::MakeChunk(str[0], str[1], str[2], str[3]); }

		//! "ASCII文字の16進数値"を数値へ
		uint32_t CharToHex(uint32_t ch);
		//! ASCII文字の4桁の16進数値を数値へ
		uint32_t StrToHex4(const char* src);
		uint32_t HexToChar(uint32_t hex);
		//! Base64変換
		int base64(char* dst, size_t n_dst, const char* src, int n);
		int base64toNum(char* dst, size_t n_dst, const char* src, int n);
		// URL変換
		int url_encode_OAUTH(char* dst, size_t n_dst, const char* src, int n);
		int url_encode(char* dst, size_t n_dst, const char* src, int n);
		// UTF-16関連
		bool utf16_isSurrogate(char16_t c);
		bool utf16_isSpace(char16_t c);
		bool utf16_isLF(char16_t c);
		bool utf16_isPrivate(char16_t c);	// 私用領域なら1，サロゲート私用領域なら2を返す(予定)
		// UTF16 <-> UTF8 相互変換
		std::u16string UTFConvertTo16(c8Str src);
		std::u16string UTFConvertTo16(c32Str src);
		std::u32string UTFConvertTo32(c16Str src);
		std::u32string UTFConvertTo32(c8Str src);
		std::string UTFConvertTo8(c16Str src);
		std::string UTFConvertTo8(c32Str src);
		void WriteData(void* pDst, char32_t val, int n);

		// nread, nwriteはバイト数ではなく文字数を表す
		// UTF変換(主に内部用)
		Code UTF16To32(char32_t src);
		Code UTF32To16(char32_t src);
		Code UTF8To32(char32_t src);
		//! 不正なシーケンスを検出すると例外を発生させる
		Code UTF8To32_s(char32_t src);
		Code UTF32To8(char32_t src);
		Code UTF8To16(char32_t src);
		Code UTF16To8(char32_t src);
	};

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
					if(_nCur > _buff.size()/sizeof(T)) {
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

	template <int N>
	struct AlignedAux {
		unsigned data : 8*N;
		unsigned offset : 8;
	};
	template <>
	struct AlignedAux<0> {
		uint8_t offset;
	};
	//! アラインメントされたメモリ領域を確保
	/*!	N: 追加情報サイズ(bytes)
		\param[in] nAlign	バイト境界 (2byte以上128byte以下)
		\param[in] size		確保したいバイト数
		\return ユーザーに返すアライン済みメモリ, 追加情報構造体 */
	template <int N>
	inline std::pair<void*, AlignedAux<N>*>	AlignedAlloc(size_t nAlign, size_t size) {
		assert(spn::Bit::Count(nAlign) == 1);		// nAlignは2の乗数倍
		assert(nAlign >= 2 && nAlign <= 128);

		using Aux = AlignedAux<N>;
		size_t sz = size + nAlign + sizeof(Aux) -1;
		uintptr_t ptr = (uintptr_t)std::malloc(sz);
		// アラインメントオフセット計算
		uint32_t ofs = nAlign - ((ptr + sizeof(Aux)) & (nAlign-1));
		// ユーザーに返すメモリ領域の前にずらした距離を記載
		Aux* aux = reinterpret_cast<Aux*>(ptr + ofs);
		aux->offset = static_cast<uint8_t>(ofs + sizeof(Aux));
		return std::make_pair(reinterpret_cast<void*>(ptr + ofs + sizeof(Aux)), aux);
	}

	//! AlignedAllocで確保したメモリの解放
	/*! \param[in] ptr	開放したいメモリのポインタ(nullは不可) */
	template <int N>
	inline void AlignedFree(void* ptr) {
		assert(ptr);

		auto iptr = reinterpret_cast<uintptr_t>(ptr);
		using Aux = AlignedAux<N>;
		Aux* aux = reinterpret_cast<Aux*>(iptr - sizeof(Aux));
		std::free(reinterpret_cast<void*>(iptr - aux->offset));
	}

	//! バイトアラインメント付きのメモリ確保
	/*! 解放はAFreeで行う */
	template <class T, class... Args>
	T* AAlloc(int nAlign, Args&&... args) {
		return new(AlignedAlloc<0>(nAlign, sizeof(T)).first) T(std::forward<Args>(args)...);
	}
	//! initializer_listによる初期化
	template <class T, class A>
	T* AAlloc(int nAlign, std::initializer_list<A>&& w) {
		return new(AlignedAlloc<0>(nAlign, sizeof(T)).first) T(std::forward<std::initializer_list<A>>(w));
	}
	//! バイトアラインメント付きの配列メモリ確保
	template <class T>
	T* AArray(int nAlign, int n) {
		// 先頭に要素数カウンタを置く
		const size_t size = (sizeof(T) + nAlign-1) & ~(nAlign-1);
		AssertP(Trap, size == sizeof(T))

		auto ret = AlignedAlloc<3>(nAlign, size*n);
		ret.second->data = n;
		uintptr_t ptr = reinterpret_cast<uintptr_t>(ret.first);
		for(int i=0 ; i<n ; i++) {
			new(reinterpret_cast<T*>(ptr)) T();
			ptr += size;
		}
		return reinterpret_cast<T*>(ret.first);
	}

	template <class T>
	void AFree(T* ptr) {
		ptr->~T();
		AlignedFree<0>(ptr);
	}
	template <class T>
	void AArrayFree(T* ptr) {
		using Aux = AlignedAux<3>;
		auto* aux = reinterpret_cast<Aux*>(reinterpret_cast<uintptr_t>(ptr) - sizeof(Aux));
		auto* uptr =reinterpret_cast<T*>(ptr);
		int n = aux->data;
		while(n != 0) {
			uptr->~T();
			++uptr;
			--n;
		}
		AlignedFree<3>(ptr);
	}

	//! アラインメントチェッカ
	template <int N, class T>
	class alignas(N) CheckAlign {
		protected:
			CheckAlign() {
				// アラインメントチェック
				AssertP(Trap, (((uintptr_t)this)&(N-1)) == 0)
			}
		private:
			static void AlignedDelete(void* ptr) {
				AFree(reinterpret_cast<T*>(ptr));
			}
			struct AlignedDeleter {
				void operator()(void* ptr) const {
					AFree(reinterpret_cast<T*>(ptr));
				}
			};
			static void ArrayDelete(void* ptr) {
				AArrayFree(reinterpret_cast<T*>(ptr));
			}
			struct ArrayDeleter {
				void operator()(void* ptr) const {
					AArrayFree(reinterpret_cast<T*>(ptr));
				}
			};

			//! initializer_listのctor
			template <class A>
			static T* _New(std::initializer_list<A>&& w) {
				return AAlloc<T>(N, std::forward<std::initializer_list<A>>(w)); }
			//! 任意の引数によるctor
			template <class... Args>
			static T* _New(Args&&... args) {
				return AAlloc<T>(N, std::forward<Args>(args)...); }
		public:
			//! アラインメント済みのメモリにオブジェクトを確保し、カスタムデリータ付きのunique_ptrとして返す
			template <class... Args>
			static std::unique_ptr<T, AlignedDeleter> NewU_Args(Args&&... args) {
				return std::unique_ptr<T, AlignedDeleter>(_New(std::forward<Args>(args)...)); }
			template <class A>
			static std::unique_ptr<T, AlignedDeleter> NewU_IL(std::initializer_list<A>&& w) {
				return std::unique_ptr<T, AlignedDeleter>(_New(std::move(w))); }
			static std::unique_ptr<T[], ArrayDeleter> ArrayU(size_t n) {
				return std::unique_ptr<T[], ArrayDeleter>(AArray<T>(N, n)); }
			//! アラインメント済みのメモリにオブジェクトを確保し、個別デリータ付きのunique_ptrとして返す
			template <class... Args>
			static std::unique_ptr<T, void (*)(void*)> NewUF_Args(Args&&... args) {
				return std::unique_ptr<T, void(*)(void*)>(_New(std::forward<Args>(args)...), AlignedDelete); }
			template <class A>
			static std::unique_ptr<T, void (*)(void*)> NewUF_IL(std::initializer_list<A>&& w) {
				return std::unique_ptr<T, void(*)(void*)>(_New(std::move(w)), AlignedDelete); }
			static std::unique_ptr<T[], void (*)(void*)> ArrayUF(size_t n) {
				return std::unique_ptr<T[], void(*)(void*)>(AArray<T>(N,n), ArrayDelete); }
			//! アラインメント済みのメモリにオブジェクトを確保し、shared_ptrとして返す
			template <class... Args>
			static std::shared_ptr<T> NewS_Args(Args&&... args) {
				return std::shared_ptr<T>(_New(std::forward<Args>(args)...), AlignedDeleter()); }
			template <class A>
			static std::shared_ptr<T> NewS_IL(std::initializer_list<A>&& w) {
				return std::shared_ptr<T>(_New(std::move(w)), AlignedDeleter()); }
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
	bool IsNear(const T& val, const T& vExcept, const T& vEps = std::numeric_limits<T>::epsilon()) {
		return IsInRange(val, vExcept-vEps, vExcept+vEps);
	}
	//! 浮動少数点数の値がNaNになっているか
	template <class T, class = typename std::enable_if<std::is_floating_point<T>::value>::type>
	bool IsNaN(const T& val) {
		return !(val>=T(0)) && !(val<T(0)); }
	//! 浮動少数点数の値がNaN又は無限大になっているか
	template <class T, class = typename std::enable_if<std::is_floating_point<T>::value>::type>
	bool IsOutstanding(const T& val) {
		auto valA = std::fabs(val);
		return valA==std::numeric_limits<float>::infinity() || IsNaN(valA); }
	//! 平面上の一点が三角形の内部に含まれるかどうかを判定する
	/*! \param vtx0 三角形の頂点0
		\param vtx1 三角形の頂点1
		\param vtx2 三角形の頂点2
		\param pos 判定する平面上の点
		\return 三角形に含まれているならtrue, 含まれていないならfalse */
	bool IsInTriangle(const Vec3& vtx0, const Vec3& vtx1, const Vec3& vtx2, const Vec3& pos);
	//! クラメルの公式で使う行列式を計算
	float CramerDet(const Vec3& v0, const Vec3& v1, const Vec3& v2);
	float CramerDet(const Vec2& v0, const Vec2& v1);
	//! 3元1次方程式の解をクラメルの公式を用いて計算
	/*! @param detInv[in] 行列式(v0,v1,v2)の逆数 */
	Vec3 CramersRule(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& a0, float detInv);
	//! 2元1次方程式を計算
	Vec2 CramersRule(const Vec2& v0, const Vec2& v1, const Vec2& a0, float detInv);
	//! グラム・シュミットの正規直交化法
	void GramSchmidtOrth(Vec3& v0, Vec3& v1, Vec3& v2);
	struct AffineParts {
		AVec3 offset,
				scale;
		AQuat rotation;
	};
	//! アフィン成分分解
	AffineParts DecompAffine(const AMat43& m);
	//! ソースコード文字列に行番号を付ける
	/*! \param[in] src 元のソースコード
		\param[in] bPrevLR 最初に改行を入れるか
		\param[in] bPostLR 最後に改行を入れるか*/
	std::string AddLineNumber(const std::string& src, bool bPrevLR, bool bPostLR);

	//! 汎用シングルトン
	template <typename T>
	class Singleton {
		private:
			static T* ms_singleton;
		public:
			Singleton() {
				assert(!ms_singleton || !"initializing error - already initialized");
				intptr_t offset = reinterpret_cast<intptr_t>(reinterpret_cast<T*>(1)) - 
									reinterpret_cast<intptr_t>(reinterpret_cast<Singleton<T>*>(reinterpret_cast<T*>(1)));
				ms_singleton = reinterpret_cast<T*>(reinterpret_cast<intptr_t>(this) + offset);
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
