#pragma once
#include <tuple>
#include "common.hpp"
#include "check_macro.hpp"

DEF_HASTYPE_T(type)
namespace spn {
	//! std::result_ofの関数対応版
	/*! result_ofのシグニチャと意味が紛らわしいが、気にしない */
	template <class, class>
	struct _ResultOf;
	template <class RT, class... Ts>
	struct _ResultOf<RT (Ts...), std::false_type> { using type = RT; };
	template <class RT, class... Ts>
	struct _ResultOf<RT (&)(Ts...), std::false_type> { using type = RT; };
	template <class RT, class... Ts>
	struct _ResultOf<RT (*)(Ts...), std::false_type> { using type = RT; };
	template <class T>
	struct _ResultOf<T, std::true_type> : std::result_of<T> {};
	template <class T>
	struct ResultOf : _ResultOf<T, HasTypeT_type_t<std::result_of<T>>> {};
	template <class T>
	using ResultOf_t = typename ResultOf<T>::type;

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

	namespace detail {
		template <class T>
		T dereference_ptr(...);
		template <class T>
		auto dereference_ptr(typename std::remove_reference<decltype(*std::declval<T>())>::type* = nullptr)
			-> typename std::remove_reference<decltype(*std::declval<T>())>::type;

		template <class T>
		auto dereference(T& t, typename std::decay<decltype(*std::declval<T>())>::type*) -> decltype(*std::declval<T>()) {
			return *t;
		}
		template <class T>
		T& dereference(T& t, ...) {
			return t;
		}
	}
	//! *で参照出来る型を取り出す
	template <class T>
	using dereference_ptr_t = decltype(detail::dereference_ptr<T>(nullptr));
	//! *で参照出来る値を取り出す (参照できない場合はそのまま)
	template <class T>
	auto dereference(T& t) -> decltype(detail::dereference(t, nullptr)) {
		return detail::dereference(t, nullptr);
	}

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
	class CType;

	//! タイプリストを逆順にした物を返す（ただしCType<CType<A,B>, C> の形になる）
	/*! CPlainと組み合わせて使う */
	template <class... Ts>
	struct CReverse;
	template <class T>
	struct CReverse<T> {
		using type = CType<T>;
	};
	template <class T, class... Ts>
	struct CReverse<T,Ts...> {
		using type = CType<typename CReverse<Ts...>::type, T>;
	};
	//! CType<CType<...>, ...> の入れ子を展開する
	template <class... Ts>
	struct CPlain {
		using type = CType<Ts...>;
	};
	template <class T>
	struct CPlain<T> {
		using type = T;
	};
	template <class... Ts0, class... Ts1>
	struct CPlain<CType<Ts0...>, Ts1...> {
		using type = typename CPlain<Ts0..., Ts1...>::type;
	};
	template <class... Ts0, class... Ts1>
	struct CPlain<CType<CType<Ts0...>, Ts1...>> {
		using type = typename CPlain<Ts0..., Ts1...>::type;
	};

	template <class... TS>
	class CType {
		private:
			template <class T>
			struct _Find {
				enum { result= TypeFind<T,0,TS...>::result };
			};

		public:
			//! インデックス指定で型を取り出す
			template <int N>
			struct At {
				static_assert(N<sizeof...(TS),"At: out of index");
				using type = typename TypeAt<N,TS...>::type;
			};
			//! 末尾に型を追加
			template <class... TS2>
			using Append = CType<TS..., TS2...>;
			//! 先頭に型を追加
			template <class... TS2>
			using Prepend = CType<TS2..., TS...>;
			//! 型のインデックス検索
			template <class T>
			struct Find : _Find<T> {
				static_assert(_Find<T>::result>=0, "Find: not found");
			};
			//! 型を持っていればintegral_constant<bool,true>
			template <class T>
			struct Has : std::integral_constant<bool, _Find<T>::result>=0> {};
			//! std::tupleに変換
			using AsTuple = std::tuple<TS...>;
			//! 要素のそれぞれについてoperator()をした結果の型リストを生成
			template <class Dummy=void>
			struct Another {
				using result = CType<decltype(TS()())...>;
			};
			//! 型リストを逆順にする
			template <class Dummy=void>
			struct Reversed {
				using type = typename CPlain<typename CReverse<TS...>::type>::type;
			};
			//! 自身の型
			using type = CType<TS...>;
			//! 別の型のテンプレート引数に使う
			template <template<class...> class T>
			struct As {
				using type = T<TS...>;
			};

			constexpr static int size = sizeof...(TS),					//!< 型リストの要素数
								sum = TypeSum<TS...>::result,			//!< 要素のサイズ合計
								maxsize = TMax<sizeof(TS)...>::result;	//!< 要素の最大サイズ
			//! タイプリストの位置に対する比較(LessThan)
			template <class T0, class T1>
			using Less = std::integral_constant<bool, TValue<Find<T0>::result, Find<T1>::result>::lesser>;
			//! タイプリストの位置に対する比較(GreaterThan)
			template <class T0, class T1>
			using Greater = std::integral_constant<bool, TValue<Find<T0>::result, Find<T1>::result>::greater>;
			//! タイプリストの位置に対する比較(Equal)
			template <class T0, class T1>
			using Equal = std::is_same<T0,T1>;
			//! タイプリストの位置に対する比較(LessThan or Equal)
			template <class T0, class T1>
			using LessEq = std::integral_constant<bool, TValue<Find<T0>::result, Find<T1>::result>::less_eq>;
			//! タイプリストの位置に対する比較(GreaterThan or Equal)
			template <class T0, class T1>
			using GreatEq = std::integral_constant<bool, TValue<Find<T0>::result, Find<T1>::result>::great_eq>;

			// 型がimcompleteなケースを考えてテンプレートクラスとしてある
			//! ビットフィールド用: 必要なビット数を計算
			//! 各タイプの::offset, ::lengthフィールドを使って計算
			template <class Dummy=void>
			struct Size {
				constexpr static int maxbit = MaxBit<TS...>::result; };
			//! インデックス指定による先頭からのサイズ累計
			template <int N, template <class> class Getter=GetSize_Normal>
			struct SumN {
				static_assert(N<=sizeof...(TS),"Sum: out of index");
				constexpr static int result = _SumN<N, Getter, TS...>::result; };
			//! タイプ指定による先頭からのサイズ累計
			template <class T, template <class> class Getter=GetSize_Normal>
			struct SumT {
				constexpr static int result = _SumT<T, Getter, TS...>::result; };

		private:
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
	};
}


