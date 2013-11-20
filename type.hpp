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
	//! インデックス指定による先頭からのサイズ累計
	template <int N, template <class> class Getter, class... Ts2>
	struct _SumN {
		constexpr static int result = 0; };
	template <int N, template <class> class Getter, class T0, class... Ts2>
	struct _SumN<N, Getter, T0, Ts2...> {
		constexpr static int result = Getter<T0>::get() * (N>0 ? 1 : 0) + _SumN<N-1, Getter, Ts2...>::result; };
	//! タイプ指定による先頭からのサイズ累計
	template <class T, template <class> class Getter, class... Ts2>
	struct _SumT;
	template <class T, template <class> class Getter, class T0, class... Ts2>
	struct _SumT<T, Getter, T0, Ts2...> {
		constexpr static int result = Getter<T0>::get() + _SumT<T, Getter, Ts2...>::result; };
	template <class T, template <class> class Getter, class... Ts2>
	struct _SumT<T, Getter, T, Ts2...> {
		constexpr static int result = 0; };
	//! 通常の型サイズ取得クラス
	template <class T>
	struct GetSize_Normal {
		static constexpr int get() { return sizeof(T); }
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
		template <class... TS2>
		using ConcatAfter = CType<TS..., TS2...>;
		template <class... TS2>
		using ConcatBefore = CType<TS2..., TS...>;

		template <class T>
		struct _Find {
			enum { result= TypeFind<T,0,TS...>::result };
		};
		template <class T>
		struct Find : _Find<T> {
			static_assert(_Find<T>::result>=0, "Find: not found");
		};
		template <class T>
		struct Has : std::integral_constant<bool, _Find<T>::result>=0> {};
		using AsTuple = std::tuple<TS...>;
		template <class Dummy=void>
		struct Another {
			using result = CType<decltype(TS()())...>;
		};
		constexpr static int size = sizeof...(TS),
							sum = TypeSum<TS...>::result,
							maxsize = TMax<sizeof(TS)...>::result;
		// タイプリストの位置に対する比較
		template <class T0, class T1>
		using Less = std::integral_constant<bool, TValue<Find<T0>::result, Find<T1>::result>::lesser>;
		template <class T0, class T1>
		using Greater = std::integral_constant<bool, TValue<Find<T0>::result, Find<T1>::result>::greater>;
		template <class T0, class T1>
		using Equal = std::is_same<T0,T1>;
		template <class T0, class T1>
		using LessEq = std::integral_constant<bool, TValue<Find<T0>::result, Find<T1>::result>::less_eq>;
		template <class T0, class T1>
		using GreatEq = std::integral_constant<bool, TValue<Find<T0>::result, Find<T1>::result>::great_eq>;

		// 型がimcompleteなケースを考えてテンプレートクラスとしてある
		template <class Dummy=void>
		struct Size {
			constexpr static int maxbit = MaxBit<TS...>::result; };
		template <int N, template <class> class Getter=GetSize_Normal>
		struct SumN {
			static_assert(N<sizeof...(TS),"Sum: out of index");
			constexpr static int result = _SumN<N, Getter, TS...>::result; };
		template <class T, template <class> class Getter=GetSize_Normal>
		struct SumT {
			constexpr static int result = _SumT<T, Getter, TS...>::result; };
	};
}
