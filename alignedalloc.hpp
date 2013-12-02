#pragma once
#include "misc.hpp"

namespace spn {
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
			pointer allocate(size_type num, typename AlignedPool<T,Align>::const_pointer hint = 0) {
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
			void deallocate(pointer p, size_type num) {
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
