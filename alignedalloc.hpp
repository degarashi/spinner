#pragma once
#include "misc.hpp"

namespace spn {
	namespace {
		namespace alignedalloc_i {
			template <int N>
			struct AlignedAux {
				unsigned data : 8*N;
				unsigned offset : 8;
			};
			template <>
			struct AlignedAux<0> {
				uint8_t offset;
			};
		}
	}
	//! アラインメントされたメモリ領域を確保
	/*!	N: 追加情報サイズ(bytes)
		\param[in] nAlign	バイト境界 (2byte以上128byte以下)
		\param[in] size		確保したいバイト数
		\return ユーザーに返すアライン済みメモリ, 追加情報構造体 */
	template <int N>
	inline std::pair<void*, alignedalloc_i::AlignedAux<N>*>	AlignedAlloc(size_t nAlign, size_t size) {
		assert(spn::Bit::Count(nAlign) == 1);		// nAlignは2の乗数倍
		assert(nAlign >= 2 && nAlign <= 128);

		using Aux = alignedalloc_i::AlignedAux<N>;
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
		using Aux = alignedalloc_i::AlignedAux<N>;
		Aux* aux = reinterpret_cast<Aux*>(iptr - sizeof(Aux));
		std::free(reinterpret_cast<void*>(iptr - aux->offset));
	}
	//! バイトアラインメント付きのメモリ確保
	/*! 解放はAFreeで行う */
	template <class T, class... Args>
	T* AAlloc(int nAlign, Args&&... args) {
		return new(AlignedAlloc<0>(nAlign, sizeof(T)).first) T(std::forward<Args>(args)...);
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
		using Aux = alignedalloc_i::AlignedAux<3>;
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
	namespace alignedalloc_i {}

	//! アラインメントチェッカ
	template <int N, class T>
	class alignas(N) CheckAlign {
		protected:
			CheckAlign() {
				// アラインメントチェック
				AssertP(Trap, (((uintptr_t)this)&(N-1)) == 0)
			}
	};
	template <class T>
	class AAllocator {
		private:
			constexpr static std::size_t NAlign = alignof(T);
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

			//! 任意の引数によるctor
			template <class... Args>
			static T* _New(Args&&... args) {
				return AAlloc<T>(NAlign, std::forward<Args>(args)...); }
		public:
			//! アラインメント済みのメモリにオブジェクトを確保し、カスタムデリータ付きのunique_ptrとして返す
			template <class... Args>
			static auto NewU(Args&&... args) {
				return std::unique_ptr<T, AlignedDeleter>(_New(std::forward<Args>(args)...)); }
			static auto ArrayU(size_t n) {
				return std::unique_ptr<T[], ArrayDeleter>(AArray<T>(NAlign, n)); }
			//! アラインメント済みのメモリにオブジェクトを確保し、関数型デリータ付きのunique_ptrとして返す
			template <class... Args>
			static auto NewUF(Args&&... args) {
				return std::unique_ptr<T, void(*)(void*)>(_New(std::forward<Args>(args)...), AlignedDelete); }
			static auto ArrayUF(size_t n) {
				return std::unique_ptr<T[], void(*)(void*)>(AArray<T>(NAlign,n), ArrayDelete); }
			//! アラインメント済みのメモリにオブジェクトを確保し、shared_ptrとして返す
			template <class... Args>
			static auto NewS(Args&&... args) {
				return std::shared_ptr<T>(_New(std::forward<Args>(args)...), AlignedDeleter()); }
	};

	template <class T, size_t Align>
	class AlignedPool {
		using OffsetType = uint8_t;
		// Alignは2の累乗でなければならない
		using Check =
			typename std::enable_if<
				spn::NType<
					CTBit::LowClear<Align>::result,
					Align
				>::equal::value
			>::type;
		// AlignはOffsetTypeで表せる範囲でなければならない
		using Check2 =
			typename std::enable_if<
				spn::NType<
					Align,
					(1 << sizeof(OffsetType)*8)
				>::less::value
			>::type;
		constexpr static int TSize = spn::TValue<sizeof(T), Align>::great;
		constexpr static int Mask = Align-1;

		public:
			// 型定義
			using size_type = size_t;
			using difference_type = ptrdiff_t;
			using pointer = T*;
			using const_pointer = const T*;
			using reference = T&;
			using const_reference = const T&;
			using value_type = T;

			// アロケータをU型にバインドする
			template <class U>
			struct rebind {
				using other = AlignedPool<U,Align>;
			};

			//! コンストラクタ
			AlignedPool() noexcept {}
			AlignedPool(const AlignedPool&) noexcept {}
			//! デストラクタ
			~AlignedPool() noexcept {}

			//! メモリを割り当てる
			pointer allocate(size_type num /*, typename AlignedPool<T,Align>::const_pointer hint = 0*/) {
				uintptr_t ret = reinterpret_cast<uintptr_t>(std::malloc(TSize * num + Align));
				uintptr_t ret2 = (ret + Align) & ~Mask;
				// オフセット記録
				*reinterpret_cast<OffsetType*>(ret2-1) = ret2 - ret;
				return reinterpret_cast<pointer>(ret2);
			}
			//! 割当て済みの領域を初期化する
			template <class... Ts>
			void construct(pointer p, Ts&&... ts) {
				new( (void*)p ) T(std::forward<Ts>(ts)...);
			}

			//! メモリを解放する
			void deallocate(pointer p, size_type /*num*/) {
				auto up = (uintptr_t)p;
				size_t diff = *reinterpret_cast<OffsetType*>(up-1);
				up -= diff;
				std::free(reinterpret_cast<void*>(up));
			}
			//! 初期化済みの領域を削除する
			void destroy(pointer p) {
				p->~T();
			}

			//! アドレスを返す
			pointer address(reference value) const { return &value; }
			const_pointer address(const_reference value) const { return &value; }
			//! 割当てることができる最大の要素数を返す
			size_type max_size() const noexcept {
				return std::numeric_limits<size_t>::max() / TSize;
			}
	};
	template <class T1, class T2, size_t A1, size_t A2>
	bool operator==(const AlignedPool<T1,A1>&, const AlignedPool<T2,A2>&) noexcept { return false; }
	template <class T, size_t Align>
	bool operator==(const AlignedPool<T,Align>&, const AlignedPool<T,Align>&) noexcept { return true; }

	template <class T1, class T2, size_t A1, size_t A2>
	bool operator!=(const AlignedPool<T1,A1>&, const AlignedPool<T2,A2>&) noexcept { return true; }
	template <class T, size_t Align>
	bool operator!=(const AlignedPool<T,Align>&, const AlignedPool<T,Align>&) noexcept { return false; }

	#define DEF_APOOL(num)	template <class T> using Alloc##num = AlignedPool<T, num>;
	DEF_APOOL(32)
	DEF_APOOL(16)
	DEF_APOOL(8)
	#undef DEF_APOOL
}
