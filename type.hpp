#pragma once
#include <tuple>
#include "common.hpp"

namespace spn {
	//! bool値による型の選択
	template <int N, class T, class F>
	struct SelectType {
		using type = F;
	};
	template <class T, class F>
	struct SelectType<1,T,F> {
		using type = T;
	};
	template <int N, template<class> class T, template<class> class F>
	struct SelectTypeT {
		template <class I>
		using type = F<I>;
	};
	template <template<class> class T, template<class> class F>
	struct SelectTypeT<1,T,F> {
		template <class I>
		using type = T<I>;
	};

	//! 型変換判定
	template <class T, class U>
	class Conversion {
		public:
			typedef char Small;
			class Big { char d[2]; };
			static Small Test(U);
			static Big Test(...);
			static T MakeT();

		public:
			enum { exists =
				sizeof(Test(MakeT())) == sizeof(Small),
				sameType = 0};
	};
	template <class T>
	class Conversion<T,T> {
		public:
			enum { exists=1, sameType=1 };
	};
	template <class T>
	class Conversion<void, T> {
		public:
			enum { exists=0, sameType=0 };
	};
	template <class T>
	class Conversion<T, void> {
		public:
			enum { exists=0, sameType=0 };
	};
	template <>
	class Conversion<void, void> {
		public:
			enum { exists=1, sameType=1 };
	};

	template <class T, class U>
	constexpr bool IsDerived() { return Conversion<const U*, const T*>::exists && !Conversion<const T*, const void*>::sameType; }

	//! 特定の型を取り出す
	template <int N, class... TS>
	struct TypeAt;
	template <int N, class T, class... TS>
	struct TypeAt<N, T, TS...> {
		typedef typename TypeAt<N-1, TS...>::type	type;
	};
	template <class T, class... TS>
	struct TypeAt<0,T,TS...> {
		typedef T type;
	};

	//! タイプリストが指定のs型を含んでいるか
	template <class T, int N, class... TS>
	struct TypeFind;
	template <class T, int N, class T0, class... TS>
	struct TypeFind<T,N,T0,TS...> {
		enum { result=TypeFind<T,N+1,TS...>::result };
	};
	template <class T, int N, class... TS>
	struct TypeFind<T,N,T,TS...> {
		enum { result=N };
	};
	template <class T, int N>
	struct TypeFind<T,N> {
		enum { result=-1 };
	};

	//! ビットフィールド用: 必要なビット数を計算
	template <class... TS>
	struct MaxBit {
		enum { result=0 };
	};
	template <class T, class... TS>
	struct MaxBit<T,TS...> {
		enum { result=TValue<T::offset+T::length, MaxBit<TS...>::result>::great };
	};
	//! タイプリストの変数を並べた時に消費するメモリ量を計算
	template <class... TS>
	struct TypeSum {
		enum { result=0 };
	};
	template <class T, class... TS>
	struct TypeSum<T, TS...> {
		enum { result=sizeof(T)+TypeSum<TS...>::result };
	};

	template <class... TS>
	struct CType {
		template <int N>
		struct At {
			static_assert(N<sizeof...(TS),"At: out of index");
			using type = typename TypeAt<N,TS...>::type;
		};
		template <bool flag, int N, class DEFAULT>
		struct __At {
			using type = DEFAULT;
		};
		template <int N, class DEFAULT>
		struct __At<true,N,DEFAULT> {
			using type = typename At<N>::type;
		};
		template <int N, class DEFAULT=void>
		struct _At {
			using type = typename __At<TValue<N,sizeof...(TS)>::lesser, N,DEFAULT>::type;
		};

		template <class T>
		struct _Find {
			enum { result= TypeFind<T,0,TS...>::result };
		};
		template <class T>
		struct Find : _Find<T> {
			static_assert(_Find<T>::result>=0, "Find: not found");
		};
		template <class T>
		struct Has {
			enum { result= (_Find<T>::result>=0) ? 1:0 };
		};
		using AsTuple = std::tuple<TS...>;
		template <class Dummy=void>
		struct Another {
			using result = CType<decltype(TS()())...>;
		};
		constexpr static int size = sizeof...(TS);
		// 型がimcompleteなケースを考えてテンプレートクラスとしてある
		template <class Dummy=void>
		struct Size {
			constexpr static int sum = TypeSum<TS...>::result,
								maxbit = MaxBit<TS...>::result;
		};
	};
}
