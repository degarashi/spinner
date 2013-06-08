#pragma once

namespace spn {
	// 型変換判定
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

	template <class... TS>
	struct CType {
		template <int N>
		struct At {
			static_assert(N<sizeof...(TS),"At: out of index");
			typedef typename TypeAt<N,TS...>::type type;
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
		enum { size= sizeof...(TS) };
	};
}
