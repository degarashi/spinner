#pragma once
#include <algorithm>
#include <tuple>
#include "../type.hpp"

namespace spn {
	//! std::tuple型に暗黙の変換できるか
	template <class T, class=typename T::Tuple>
	std::true_type IsConvertibleToTuple(T*);
	std::false_type IsConvertibleToTuple(...);

	//! std::tupleに変換可能か
	template <class T>
	struct IsTupleBased : decltype(IsConvertibleToTuple((T*)nullptr)) {};
	//! ベースのstd::tuple型を取り出す
	//! std::tupleならそのまま返す
	template <class... Ts>
	auto GetTuple(std::tuple<Ts...>*) -> std::tuple<Ts...>;
	// メンバにTupleを持っていればそれを返す
	template <class T>
	auto GetTuple(T* ptr, typename T::Tuple* tag=nullptr) -> decltype(GetTuple(tag));

	template <class CB, class... Ts>
	void TupleForEach(CB&& cb, Ts&&... ts);
	namespace detail {
		template <class OP, class A0, class... Args>
		void TupleForEach(OP&& op, std::integral_constant<int,0>, A0&& a0, Args&&... args);
		template <class OP, int N, class... Args>
		void TupleForEach(OP&& op, std::integral_constant<int,N>, Args&&... args);

		static void* Enabler;
		//! 与えられた引数でコールバック関数を呼ぶ
		template <class CB, class... Ts,
					// 何れもstd::tuple型ではない
					typename std::enable_if_t<
						And_t<
							(!IsTupleBased<
								std::decay_t<Ts>
							>::value)...
						>::value
					>*& = Enabler
				>
		void CallCB(CB&& cb, Ts&&... ts) {
			cb(std::forward<Ts>(ts)...);
		}
		template <class CB, class... Ts,
					// 全てstd::tuple型
					typename std::enable_if_t<
								And_t<
									IsTupleBased<
										std::decay_t<Ts>
									>::value...
								>::value
					>*& = Enabler
				>
		void CallCB(CB&& cb, Ts&&... ts) {
			::spn::TupleForEach(cb, std::forward<Ts>(ts)...);
		}

		// 終端処理
		template <class OP, class A0, class... Args>
		void TupleForEach(OP&& /*op*/, std::integral_constant<int,0>, A0&& /*a0*/, Args&&... /*args*/) {}
		/*!
			\tparam	CB		コールバック関数
			\tparam N		処理をするTupleの位置
			\tparam Args	Tuple列
		*/
		template <class CB, int N, class... Args>
		void TupleForEach(CB&& cb, std::integral_constant<int,N>, Args&&... args) {
			CallCB(
				cb,
				std::get<N-1>(
					(decltype(GetTuple((std::decay_t<Args>*)nullptr))&)args
				)...
			);
			TupleForEach(
				cb,
				std::integral_constant<int,N-1>(),
				std::forward<Args>(args)...
			);
		}
	}
	/*!
		Tupleのサイズを計算して内部関数に受け渡す
		\tparam CB	コールバック関数
		\tparam Ts	Tuple列
	*/
	template <class CB, class... Ts>
	void TupleForEach(CB&& cb, Ts&&... ts) {
		using T = typename FirstType<std::decay_t<Ts>...>::type;
		using Tuple = decltype(GetTuple((T*)nullptr));
		constexpr int sz = std::tuple_size<Tuple>::value;
		detail::TupleForEach(
			cb,
			std::integral_constant<int,sz>(),
			std::forward<Ts>(ts)...
		);
	}

	static void* Enabler2;
	template <class T, typename std::enable_if_t< !IsTupleBased< std::decay_t<T> >::value>*& = Enabler2 >
	void TupleZeroFill(T& t) {
		t = 0;
	}
	template <class T, typename std::enable_if_t< IsTupleBased< std::decay_t<T> >::value>*& = Enabler2 >
	void TupleZeroFill(T& p) {
		TupleForEach([](auto& a){
			TupleZeroFill(a);
		}, p);
	}
}

#define DefOp(op) \
	template <class T0, class T1, \
			class=std::enable_if_t< spn::IsTupleBased< std::decay_t<T0> >::value>, \
			class=std::enable_if_t< spn::IsTupleBased< std::decay_t<T1> >::value>> \
	decltype(auto) operator op##= (T0& p0, const T1& p1) { \
		const auto fn = [](auto& a, const auto& b){ a op##= b; }; \
		return spn::TupleForEach(fn, p0, p1); \
	} \
	template <class T0, class T1, \
			class=std::enable_if_t< spn::IsTupleBased< std::decay_t<T0> >::value>, \
			class=std::enable_if_t< spn::IsTupleBased< std::decay_t<T1> >::value>> \
	auto operator op (const T0& p0, const T1& p1) { \
		const auto fn = [](const auto& a, const auto& b){ return a op b; }; \
		auto tmp = p0; \
		spn::TupleForEach(fn, tmp, p1); \
		return tmp; \
	}
DefOp(+)
DefOp(-)
DefOp(*)
DefOp(/)
DefOp(%)
#undef DefOp
