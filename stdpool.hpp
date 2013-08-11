#pragma once
#include "boost/pool/pool.hpp"

namespace spn {
	//! 整数値を型として扱う
	template <int N>
	struct Number2Type {};

	//! boost::object_poolをstd::allocatorとして扱うクラス
	template <class T, class IDType, size_t DefNum>
	class StdPool {
		public:
			static boost::pool<>	s_pool;

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
				using other = StdPool<U,IDType,DefNum>;
			};

			//! コンストラクタ
			StdPool() noexcept {}
			StdPool(const StdPool&) noexcept {}
			//! デストラクタ
			~StdPool() throw(){}

			//! メモリを割り当てる
			pointer allocate(size_type num, typename StdPool<T,IDType,DefNum>::const_pointer hint = 0) {
				if(num == 1)
					return reinterpret_cast<pointer>(s_pool.malloc());
				return reinterpret_cast<pointer>(s_pool.ordered_malloc(num));
			}
			//! 割当て済みの領域を初期化する
			void construct(pointer p, const T& value) {
				new( (void*)p ) T(value);
			}

			//! メモリを解放する
			void deallocate(pointer p, size_type num) {
				if(num == 1)
					s_pool.free(p);
				else
					s_pool.ordered_free(p);
			}
			//! 初期化済みの領域を削除する
			void destroy(pointer p) {
				p->~T();
			}

			//! アドレスを返す
			pointer address(reference value) const { return &value; }
			const_pointer address(const_reference value) const { return &value; }
			//! 割当てることができる最大の要素数を返す
			size_type max_size() const throw() {
				return std::numeric_limits<size_t>::max() / sizeof(T);
			}
	};
	template <class T, class IDType, size_t DefNum>
	boost::pool<> StdPool<T,IDType,DefNum>::s_pool(sizeof(T), DefNum);

	template <class T1, class T2, class ID1, class ID2, size_t DN1, size_t DN2>
	bool operator==(const StdPool<T1,ID1,DN1>&, const StdPool<T2,ID2,DN2>&) noexcept { return false; }
	template <class T, class ID, size_t DN>
	bool operator==(const StdPool<T,ID,DN>&, const StdPool<T,ID,DN>&) noexcept { return true; }

	template <class T1, class T2, class ID1, class ID2, size_t DN1, size_t DN2>
	bool operator!=(const StdPool<T1,ID1,DN1>&, const StdPool<T2,ID2,DN2>&) noexcept { return true; }
	template <class T, class ID, size_t DN>
	bool operator!=(const StdPool<T,ID,DN>&, const StdPool<T,ID,DN>&) noexcept { return false; }
}
