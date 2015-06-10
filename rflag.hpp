#pragma once
#include "type.hpp"
#include "error.hpp"
#include <boost/preprocessor.hpp>

namespace spn {
	template <class... Ts>
	struct ValueHolder {
		template <class T2>
		void ref(T2*) {}
		template <class T2>
		void cref(T2*) const {}
	};
	template <class T, class... Ts>
	struct ValueHolder<T, Ts...> : ValueHolder<Ts...> {
		using base = ValueHolder<Ts...>;
		using value_type = typename T::value_type;
		value_type	value;

		value_type& ref(T*) { return value; }
		const value_type& cref(T*) const { return value; }
		template <class T2>
		decltype(std::declval<base>().ref((T2*)nullptr)) ref(T2* p) {			// decltype(base::ref(p))
			return base::ref(p); }
		template <class T2>
		decltype(std::declval<base>().cref((T2*)nullptr)) cref(T2* p) const {	// decltype(base::cref(p))
			return base::cref(p); }
	};
	using RFlagValue_t = uint_fast32_t;
	struct RFlagRet {
		RFlagValue_t	flagOr;		//!< FlagValueのOr差分 (一緒に更新された変数を伝達する時に使用)
		bool			bCancel;	//!< trueなら自身の更新フラグを取り下げる (次回も更新がかかる)
	};
	using AcCounter_t = uint_fast32_t;
	//! 変数が更新された時の累積カウンタの値を後で比較するためのラッパークラス
	template <class T>
	struct AcWrapper : T {
		mutable AcCounter_t	ac_counter = ~0;
		using T::T;
		using T::operator =;
	};
	//! キャッシュ変数の自動管理クラス
	template <class Class, class... Ts>
	class RFlag : public ValueHolder<Ts...> {
		private:
			//! 更新処理がかかる度にインクリメントされるカウンタ
			mutable AcCounter_t _accum[sizeof...(Ts)];
			using base = ValueHolder<Ts...>;
			using ct_base = spn::CType<Ts...>;
			template <class T>
			static T* _NullPtr() { return reinterpret_cast<T*>(0); }
			template <class T>
			using cref_type = decltype(std::declval<base>().cref(_NullPtr<T>()));
			template <class T>
			using ref_type = typename std::decay<cref_type<T>>::type&;

			mutable RFlagValue_t _rflag;
			//! integral_constantの値がtrueなら引数テンプレートのOr()を返す
			template <class T0>
			static constexpr RFlagValue_t _Add_If(std::integral_constant<bool,false>) { return 0; }
			template <class T0>
			static constexpr RFlagValue_t _Add_If(std::integral_constant<bool,true>) { return OrLH<T0>(); }
			template <class T, int N>
			static constexpr RFlagValue_t _IterateLH(std::integral_constant<int,N>) {
				using T0 = typename ct_base::template At<N>::type;
				using T0Has = typename T0::template Has<T>;
				return _Add_If<T0>(T0Has()) |
						_IterateLH<T>(std::integral_constant<int,N+1>());
			}
			template <class T>
			static constexpr RFlagValue_t _IterateLH(std::integral_constant<int,ct_base::size>) { return 0; }

			template <class T, int N>
			static constexpr RFlagValue_t _IterateHL(std::integral_constant<int,N>) {
				using T0 = typename T::template At<N>::type;
				return OrHL<T0>() | _IterateHL<T>(std::integral_constant<int,N-1>());
			}
			template <class T>
			static constexpr RFlagValue_t _IterateHL(std::integral_constant<int,-1>) { return 0; }

			template <class T, class CB>
			cref_type<T> _refresh(const Class* self, CB&& callback) const {
				const base* ptrC = this;
				base* ptr = const_cast<base*>(ptrC);
				constexpr auto TFlag = Get<T>();
				RFlagRet ret = self->_refresh(ptr->ref(_NullPtr<T>()), _NullPtr<T>());
				_rflag |= ret.flagOr;
				_rflag &= ret.bCancel ? ~0 : ~TFlag;
				callback(GetFlagIndex<T>());
				AssertP(Trap, !(_rflag & (OrHL<T>() & ~TFlag)), "refresh flag was not cleared correctly")
				// 累積カウンタをインクリメント
				++_accum[GetFlagIndex<T>()];
				return ptrC->cref(_NullPtr<T>());
			}
			//! 変数型の格納順インデックス
			template <class TA>
			static constexpr int GetFlagIndex() {
				return ct_base::template Find<TA>::result;
			}
			//! 変数型を示すフラグ
			template <class TA>
			static constexpr RFlagValue_t GetFlagSingle() {
				return 1 << GetFlagIndex<TA>();
			}
			//! 引数の変数フラグをORで合成
			template <class... TsA>
			static constexpr RFlagValue_t _Sum(std::integral_constant<int,0>) { return 0; }
			template <class TA, class... TsA, int N>
			static constexpr RFlagValue_t _Sum(std::integral_constant<int,N>) {
				return GetFlagSingle<TA>() | _Sum<TsA...>(std::integral_constant<int,N-1>());
			}

			//! 変数に影響するフラグを立てる
			template <class... TsA>
			void _setFlag(std::integral_constant<int,0>) {}
			template <class T, class... TsA, int N>
			void _setFlag(std::integral_constant<int,N>) {
				// 自分以下の階層はフラグを下ろす
				_rflag &= ~OrHL<T>();
				// 自分の階層より上の変数は全てフラグを立てる
				_rflag |= OrLH<T>() & ~Get<T>();
				// 累積カウンタをインクリメント
				++_accum[GetFlagIndex<T>()];
				_setFlag<TsA...>(std::integral_constant<int,N-1>());
			}

		public:
			RFlag() {
				resetAll();
			}
			//! 全てのキャッシュを無効化
			void resetAll() {
				for(auto& a : _accum)
					a = 0;
				_rflag = All();
			}
			//! 引数の変数を示すフラグ
			template <class... TsA>
			static constexpr RFlagValue_t Get() {
				return _Sum<TsA...>(std::integral_constant<int,sizeof...(TsA)>());
			}
			//! 変数全てを示すフラグ値
			static constexpr RFlagValue_t All() {
				return (1 << (sizeof...(Ts)+1)) -1;
			}
			//! 累積カウンタ値取得
			template <class TA>
			AcCounter_t getAcCounter() const {
				return _accum[GetFlagIndex<TA>()];
			}
			//! 現在のフラグ値
			RFlagValue_t GetFlag() const {
				return _rflag;
			}
			//! フラグのテストだけする
			template <class... TsA>
			RFlagValue_t Test() const {
				return GetFlag() & Get<TsA...>();
			}

			//! 自分より上の階層のフラグ (Low -> High)
			template <class T>
			static constexpr RFlagValue_t OrLH() {
				// TypeListを巡回、A::Has<T>ならOr<A>()をたす
				return Get<T>() | _IterateLH<T>(std::integral_constant<int,0>());
			}
			//! 自分以下の階層のフラグ (High -> Low)
			template <class T>
			static constexpr RFlagValue_t OrHL() {
				return Get<T>() | _IterateHL<T>(std::integral_constant<int,T::size-1>());
			}

			//! 更新フラグだけを立てる
			/*! 累積カウンタも更新 */
			template <class... TsA>
			void setFlag() {
				_setFlag<TsA...>(std::integral_constant<int,sizeof...(TsA)>());
			}
			//! 必要に応じて更新をかけつつ、参照を返す
			template <class T>
			cref_type<T> get(const Class* self) const {
				if(!(_rflag & Get<T>()))
					return base::cref(_NullPtr<T>());
				return _refresh<T>(self, [](auto){});
			}
			//! 更新フラグはそのままに参照を返す
			template <class T>
			ref_type<T> ref() const {
				const auto& ret = base::cref(_NullPtr<T>());
				return const_cast<typename std::decay<decltype(ret)>::type&>(ret);
			}
			//! 更新フラグを立てつつ参照を返す
			template <class T>
			ref_type<T> refF() {
				// 更新フラグをセット
				setFlag<T>();
				return ref<T>();
			}
			//! キャッシュ機能付きの値を変更する際に使用
			template <class T, class TA>
			void set(TA&& t) {
				refF<T>() = std::forward<TA>(t);
			}
			//! 指定した変数型を更新しつつ、キャッシュがrefreshされた時にコールバック関数を呼ぶ
			template <class CB, class... TsA>
			void refresh(const Class* /*self*/, CB&& /*cb*/, CType<TsA...>) const {}
			template <class CB, class TA, class... TsA>
			void refresh(const Class* self, CB&& cb, CType<TA, TsA...>) const {
				if(_rflag & Get<TA>())
					_refresh<TA>(self, std::forward<CB>(cb));
				refresh(self, std::forward<CB>(cb), CType<TsA...>());
			}
			//! リフレッシュコールだけをする (複数同時指定可能)
			template <class... TsA>
			void refresh(const Class* self, CType<TsA...> t) const {
				refresh(self, [](auto...){}, t);
			}
	};

	#define RFLAG(clazz, seq) \
		using RFlag_t = spn::RFlag<clazz, BOOST_PP_SEQ_ENUM(seq)>; \
		RFlag_t _rflag; \
		template <class T, class... Ts> \
		friend class spn::RFlag;
	#define RFLAG_RVALUE_BASE(name, valueT, ...) \
		struct name : spn::CType<__VA_ARGS__> { \
			using value_type = valueT; \
			value_type value; };
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
		::spn::RFlagRet _refresh(valueT&, name*) const;
	#define RFLAG_RVALUE_(name, valueT) \
		::spn::RFlagRet _refresh(valueT&, name*) const { return {}; }
	#define RFLAG_GETMETHOD(name) auto get##name() const -> decltype(_rflag.template get<name>(this)) { return _rflag.template get<name>(this); }
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

	#define RFLAG_FUNC3(z, func, seq)	\
		BOOST_PP_IF( \
			BOOST_PP_LESS_EQUAL( \
				BOOST_PP_SEQ_SIZE(seq), \
				2 \
			), \
			func, \
			NOTHING_ARG \
		)(BOOST_PP_SEQ_ELEM(0, seq))

	//! キャッシュ変数にset()メソッドを定義
	#define RFLAG_SETMETHOD(name) \
		template <class TArg> \
		void BOOST_PP_CAT(set, name(TArg&& t)) { _rflag.template set<name>(std::forward<TArg>(t)); }
	//! 最下層のキャッシュ変数に一括でset()メソッドを定義
	#define RFLAG_SETMETHOD_S(seq) \
		BOOST_PP_SEQ_FOR_EACH( \
			RFLAG_FUNC3, \
			RFLAG_SETMETHOD, \
			seq \
		)

	//! キャッシュ変数にref()メソッドを定義
	#define RFLAG_REFMETHOD(name) \
		auto& BOOST_PP_CAT(ref, name()) { return _rflag.template refF<name>(); }
	//! 最下層のキャッシュ変数に一括でref()メソッドを定義
	#define RFLAG_REFMETHOD_S(seq) \
		BOOST_PP_SEQ_FOR_EACH( \
			RFLAG_FUNC3, \
			RFLAG_REFMETHOD, \
			seq \
		)

	/* class MyClass {
		RFLAG_RVALUE(Value0, Type0)
		RFLAG_RVALUE(Value0, Type0, Depends....) をクラス内に変数分だけ書く
		更新関数を RFlagRet _refresh(Type0&, Value0*) const; の形で記述
		RFLAG(MyClass, Value0, Value0...)
		public:
			// 参照用の関数
			RFLAG_GETMETHOD(Value0)
			RFLAG_GETMETHOD(Value1)

		参照する時は _rflag.template get<Value0>(this) とやるか、
		getValue0()とすると依存変数の自動更新
		_rfvalue.template ref<Value0>(this)とすれば依存関係を無視して取り出せる

		--- 上記の簡略系 ---
		#define SEQ ((Value0)(Type0))((Value1)(Type1)(Depend))
		private:
			RFLAG_S(MyClass, SEQ)
		public:
			RFLAG_GETMETHOD_S(SEQ) */

	#define VARIADIC_FOR_EACH_FUNC(z, n, data)	BOOST_PP_TUPLE_ELEM(0, data)(1, BOOST_PP_TUPLE_ELEM(1,data), BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(n,2),data))
	#define VARIADIC_FOR_EACH(macro, data, ...)	BOOST_PP_REPEAT(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), VARIADIC_FOR_EACH_FUNC, BOOST_PP_VARIADIC_TO_TUPLE(macro, data, __VA_ARGS__))
}

