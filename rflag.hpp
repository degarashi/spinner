#pragma once
#include "type.hpp"
#define BOOST_PP_VARIADICS 1
#include <boost/preprocessor.hpp>

namespace spn {
	//! キャッシュ変数の自動管理クラス
	template <class Class, class... Ts>
	class RFlag : public spn::CType<Ts...> {
		public:
			using FlagValue = uint32_t;
		private:
			using base = spn::CType<Ts...>;
			mutable FlagValue _rflag = All();

			//! integral_constantの値がtrueなら引数テンプレートのOr()を返す
			template <class T0>
			static constexpr FlagValue _Add_If(std::integral_constant<bool,false>) { return 0; }
			template <class T0>
			static constexpr FlagValue _Add_If(std::integral_constant<bool,true>) { return OrLH<T0>(); }
			template <class T, int N>
			static constexpr FlagValue _IterateLH(std::integral_constant<int,N>) {
				using T0 = typename base::template At<N>::type;
				using T0Has = typename T0::template Has<T>;
				return _Add_If<T0>(T0Has()) |
						_IterateLH<T>(std::integral_constant<int,N+1>());
			}
			template <class T>
			static constexpr FlagValue _IterateLH(std::integral_constant<int,base::size>) { return 0; }

			template <class T, int N>
			static constexpr FlagValue _IterateHL(std::integral_constant<int,N>) {
				using T0 = typename base::template At<N>::type;
				return OrHL<T0>() | _IterateHL<T>(std::integral_constant<int,N-1>());
			}
			template <class T>
			static constexpr FlagValue _IterateHL(std::integral_constant<int,-1>) { return 0; }

			template <class T>
			static T* _GetNull() { return reinterpret_cast<T*>(0); }

			template <class T, int N>
			auto _refresh(const Class* self, std::integral_constant<int,N>) const -> const decltype(T::value)& {
				using TN = typename T::template At<N>::type;
				// フラグをチェック
				if(_rflag & Get<TN>())
					_rflag &= ~(self->_refresh(ref<TN>(self), _GetNull<TN>()) | Get<TN>());
				return _refresh<T>(self, std::integral_constant<int,N-1>());
			}
			template <class T>
			auto _refresh(const Class* self, std::integral_constant<int,-1>) const -> const decltype(T::value)& {
				if(_rflag & Get<T>())
					_rflag &= ~(self->_refresh(ref<T>(self), _GetNull<T>()) | Get<T>());
				return self->_get(_GetNull<T>());
			}

		public:
			template <class T>
			static constexpr FlagValue Get() {
				return 1 << base::template Find<T>::result;
			}
			static constexpr FlagValue All() {
				return (1 << (sizeof...(Ts)+1)) -1;
			}

			template <class T>
			static constexpr FlagValue OrLH() {
				// TypeListを巡回、A::Has<T>ならOr<A>()をたす
				return Get<T>() | _IterateLH<T>(std::integral_constant<int,0>());
			}
			template <class T>
			static constexpr FlagValue OrHL() {
				return Get<T>() | _IterateHL<T>(std::integral_constant<int,T::size-1>());
			}

			//! 更新フラグだけを立てる
			template <class T>
			void setFlag() {
				_rflag |= Get<T>();
			}
			template <class T>
			auto get(const Class* self) const -> decltype(self->_get(_GetNull<T>())) {
				if(!(OrHL<T>() & _rflag))
					return self->_get(_GetNull<T>());
				return _refresh<T>(self, std::integral_constant<int,T::size-1>());
			}
			template <class T>
			auto ref(const Class* self) const -> typename std::decay<decltype(self->_get(_GetNull<T>()))>::type& {
				// 更新チェックなしで参照を返す
				const auto& ret = self->_get(_GetNull<T>());
				return const_cast<typename std::decay<decltype(ret)>::type&>(ret);
			}
			//! キャッシュ機能付きの値を変更する際に使用
			template <class T, class TA>
			void set(Class* self, TA&& t) {
				// 更新フラグを立てる
				setFlag<T>();
				ref<T>(self) = std::forward<TA>(t);
			}
	};

	#define RFLAG(clazz, seq) \
		using RFlag_t = spn::RFlag<clazz, BOOST_PP_SEQ_ENUM(seq)>; \
		RFlag_t _rflag; \
		template <class T, class... Ts> \
		friend class spn::RFlag;
	#define RFLAG_RVALUE_BASE(name, valueT, ...) \
		struct name : spn::CType<__VA_ARGS__> { \
			valueT value; } name##_value; \
		const valueT& _get(name*) const { return name##_value.value; }
	#define RFLAG_RVALUE(name, ...) \
			RFLAG_RVALUE_BASE(name, __VA_ARGS__) \
			BOOST_PP_IF( \
				BOOST_PP_LESS_EQUAL( \
					BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), \
					1 \
				), \
				RFLAG_RVALUE_, \
				RFLAG_RVALUE_D_ \
			)(name, __VA_ARGS__)
	#define RFLAG_RVALUE_D_(name, valueT, ...) \
		uint32_t _refresh(valueT&, name*) const;
	#define RFLAG_RVALUE_(name, valueT) \
		uint32_t _refresh(valueT&, name*) const { return 0; }
	#define RFLAG_GETMETHOD(name) auto get##name() const -> decltype(_rflag.get<name>(this)) { return _rflag.get<name>(this); }
	#define PASS_THROUGH(func, ...)		func(__VA_ARGS__)
	#define RFLAG_FUNC(z, data, elem)	PASS_THROUGH(RFLAG_RVALUE, BOOST_PP_SEQ_ENUM(elem))
	#define SEQ_GETFIRST(z, data, elem)	(BOOST_PP_SEQ_ELEM(0,elem))
	#define RFLAG_S(clazz, seq)	\
		BOOST_PP_SEQ_FOR_EACH( \
			RFLAG_FUNC, \
			BOOST_PP_NIL, \
			seq \
		) \
		RFLAG( \
			clazz, \
			BOOST_PP_SEQ_FOR_EACH( \
				SEQ_GETFIRST, \
				BOOST_PP_NIL, \
				seq \
			) \
		)

	#define RFLAG_FUNC2(z, data, elem)	RFLAG_GETMETHOD(elem)
	#define RFLAG_GETMETHOD_S(seq) \
		BOOST_PP_SEQ_FOR_EACH( \
			RFLAG_FUNC2, \
			BOOST_PP_NIL, \
			BOOST_PP_SEQ_FOR_EACH( \
				SEQ_GETFIRST, \
				BOOST_PP_NIL, \
				seq \
			) \
		)
	/* class MyClass {
		RFLAG_RVALUE(Value0, Type0)
		RFLAG_RVALUE(Value0, Type0, Depends....) をクラス内に変数分だけ書く
		更新関数を uint32_t _refresh(Type0&, Value0*) const; の形で記述
		RFLAG(MyClass, Value0, Value0...)
		public:
			// 参照用の関数
			RFLAG_GETMETHOD(Value0)
			RFLAG_GETMETHOD(Value1)

		参照する時は _rflag.get<Value0>(this) とやるか、
		getValue0()とすると依存変数の自動更新
		_rfvalue.ref<Value0>(this)とすれば依存関係を無視して取り出せる

		--- 上記の簡略系 ---
		#define SEQ ((Value0)(Type0))((Value1)(Type1)(Depend))
		private:
			RFLAG_S(MyClass, SEQ)
		public:
			RFLAG_GETMETHOD_S(SEQ) */

	#define VARIADIC_FOR_EACH_FUNC(z, n, data)	BOOST_PP_TUPLE_ELEM(0, data)(1, BOOST_PP_TUPLE_ELEM(1,data), BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(n,2),data))
	#define VARIADIC_FOR_EACH(macro, data, ...)	BOOST_PP_REPEAT(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), VARIADIC_FOR_EACH_FUNC, BOOST_PP_VARIADIC_TO_TUPLE(macro, data, __VA_ARGS__))
}

