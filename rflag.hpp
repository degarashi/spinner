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
		bool			bRefreshed;		//!< 変数値の更新がされた時はtrue
		RFlagValue_t	flagOr;			//!< FlagValueのOr差分 (一緒に更新された変数を伝達する時に使用)
	};
	using AcCounter_t = uint_fast32_t;
	//! 変数が更新された時の累積カウンタの値を後で比較するためのラッパークラス
	template <class T, class Getter, class... Ts>
	class AcWrapper : public T {
		public:
			using Flag_t = CType<Ts...>;
			using Getter_t = Getter;
			using Counter_t = typename Getter::counter_t;
		private:
			mutable AcCounter_t ac_counter[sizeof...(Ts)];
			mutable Counter_t user_counter[sizeof...(Ts)];
		public:
			template <class... Args>
			explicit AcWrapper(Args&&... args):
				T(std::forward<Args>(args)...)
			{
				for(auto& a : ac_counter)
					a = ~0;
				for(auto& a : user_counter)
					a = ~0;
			}
			AcWrapper& operator = (const T& t) {
				static_cast<T&>(*this) = t;
				return *this;
			}
			AcWrapper& operator = (T&& t) {
				static_cast<T&>(*this) = std::move(t);
				return *this;
			}
			template <class Type>
			AcCounter_t& refAc() const {
				return ac_counter[Flag_t::template Find<Type>::result];
			}
			template <class Type>
			Counter_t& refUserAc() const {
				return user_counter[Flag_t::template Find<Type>::result];
			}
	};
	//! TがAcWrapperテンプレートクラスかをチェック
	template <class T>
	struct IsAcWrapper : std::false_type {};
	template <class... Ts>
	struct IsAcWrapper<AcWrapper<Ts...>> : std::true_type {};

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
			// タグを引数として実際の変数型を取り出す
			template <class T>
			using cref_type = decltype(std::declval<base>().cref(_NullPtr<T>()));
			template <class T>
			using ref_type = typename std::decay<cref_type<T>>::type&;
			template <class T>
			using cptr_type = const typename std::decay<cref_type<T>>::type*;
			template <class T>
			using CRef_p = std::pair<cref_type<T>, RFlagValue_t>;
			//! キャッシュフラグを格納する変数
			mutable RFlagValue_t _rflag;

			//! 整数const型
			template <int N>
			using IConst = std::integral_constant<int,N>;
			//! integral_constantの値がtrueなら引数テンプレートのOr()を返す
			template <class T0>
			static constexpr RFlagValue_t _Add_If(std::false_type) { return 0; }
			template <class T0>
			static constexpr RFlagValue_t _Add_If(std::true_type) { return OrLH<T0>(); }
			//! 下位の変数に影響される上位の変数のフラグを計算
			template <class T, int N>
			static constexpr RFlagValue_t __IterateLH(IConst<N>) {
				using T0 = typename ct_base::template At<N>::type;
				using T0Has = typename T0::template Has<T>;
				return _Add_If<T0>(T0Has()) |
						__IterateLH<T>(IConst<N+1>());
			}
			template <class T>
			static constexpr RFlagValue_t __IterateLH(IConst<ct_base::size>) { return 0; }
			template <class T>
			static constexpr RFlagValue_t _IterateLH() { return __IterateLH<T>(IConst<0>()); }

			//! 上位の変数が使用する下位の変数のフラグを計算
			template <class T, int N>
			static constexpr RFlagValue_t __IterateHL(IConst<N>) {
				using T0 = typename T::template At<N>::type;
				return OrHL<T0>() | __IterateHL<T>(IConst<N-1>());
			}
			template <class T>
			static constexpr RFlagValue_t __IterateHL(IConst<-1>) { return 0; }
			template <class T>
			static constexpr RFlagValue_t _IterateHL() { return __IterateHL<T>(IConst<T::size-1>()); }

			//! 上位の変数が使用する下位の変数のフラグを計算 (直下のノードのみ)
			template <class T, int N>
			static constexpr RFlagValue_t __IterateHL0(IConst<N>) {
				using T0 = typename T::template At<N>::type;
				return Get<T0>() | __IterateHL0<T>(IConst<N-1>());
			}
			template <class T>
			static constexpr RFlagValue_t __IterateHL0(IConst<-1>) { return 0; }
			template <class T>
			static constexpr RFlagValue_t _IterateHL0() { return __IterateHL0<T>(IConst<T::size-1>()); }

			//! キャッシュ変数Tを更新
			template <class T>
			CRef_p<T> _refreshSingle(const Class* self) const {
				const base* ptrC = this;
				base* ptr = const_cast<base*>(ptrC);
				constexpr auto TFlag = Get<T>();
				// 親クラスのRefresh関数を呼ぶ
				RFlagRet ret = self->_refresh(ptr->ref(_NullPtr<T>()), _NullPtr<T>());
				// 一緒に更新された変数フラグを立てる
				_rflag |= ret.flagOr;
				_rflag &= ~TFlag;
				AssertP(Trap, !(_rflag & (OrHL0<T>() & ~TFlag & ~ACFlag)), "refresh flag was not cleared correctly")
				// Accumulationクラスを継承している変数は常に更新フラグを立てておく
				_rflag |= ACFlag;
				// 累積カウンタをインクリメント
				++_accum[GetFlagIndex<T>()];
				// 変数が更新された場合にはsecondに当該変数のフラグを返す
				return CRef_p<T>(
							ptrC->cref(_NullPtr<T>()),
							ret.bRefreshed ? TFlag : 0
						);
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
			static constexpr RFlagValue_t _Sum(IConst<0>) { return 0; }
			template <class TA, class... TsA, int N>
			static constexpr RFlagValue_t _Sum(IConst<N>) {
				return GetFlagSingle<TA>() | _Sum<TsA...>(IConst<N-1>());
			}

			//! 変数に影響するフラグを立てる
			template <class... TsA>
			void __setFlag(IConst<0>) {}
			template <class T, class... TsA, int N>
			void __setFlag(IConst<N>) {
				// 自分の階層より上の変数は全てフラグを立てる
				_rflag |= OrLH<T>();
				_rflag &= ~Get<T>();
				// Accumulationクラスを継承している変数は常に更新フラグを立てておく
				_rflag |= ACFlag;
				// 累積カウンタをインクリメント
				++_accum[GetFlagIndex<T>()];
				__setFlag<TsA...>(IConst<N-1>());
			}
			template <class... TsA>
			void _setFlag() {
				__setFlag<TsA...>(IConst<sizeof...(TsA)>());
			}

			//! キャッシュ変数ポインタをstd::tupleにして返す
			template <int N, class Tup>
			RFlagValue_t _getAsTuple(const Class* /*self*/, const Tup& /*dst*/) const { return 0; }
			template <int N, class Tup, class T, class... TsA>
			RFlagValue_t _getAsTuple(const Class* self, Tup& dst, T*, TsA*... remain) const {
				auto ret = refresh<T>(self);
				std::get<N>(dst) = &ret.first;
				return ret.second | _getAsTuple<N+1>(self, dst, remain...);
			}
			template <int N, class Acc, class Tup>
			RFlagValue_t _getWithCheck(const Class*, const Acc&, const Tup&) const { return 0; }
			template <int N, class Acc, class Tup, class T, class... TsA>
			RFlagValue_t _getWithCheck(const Class* self, Acc& acc, Tup& dst, T*, TsA*... remain) const {
				auto ret = refresh<T>(self);
				std::get<N>(dst) = &ret.first;
				RFlagValue_t flag = ret.second;
				flag |= _getIfFlagDifferent<T>(acc, ret.first, typename Acc::Flag_t::template Has<T>());
				return flag | _getWithCheck<N+1>(self, acc, dst, remain...);
			}
			template <class T, class Acc>
			RFlagValue_t _getIfFlagDifferent(Acc& acc, cref_type<T> v, std::true_type) const {
				using C = typename Acc::Counter_t;
				using G = typename Acc::Getter_t;
				if(CompareAndSet<C>(acc.template refUserAc<T>(), G()(v, _NullPtr<T>())) |
					CompareAndSet(acc.template refAc<T>(), getAcCounter<T>()))
					return Get<T>();
				return 0;
			}
			template <class T, class Acc>
			RFlagValue_t _getIfFlagDifferent(Acc&, cref_type<T>, std::false_type) const {
				return 0;
			}

		public:
			template <class T>
			constexpr static T ReturnIf(T flag, std::true_type) { return flag; }
			template <class T>
			constexpr static T ReturnIf(T, std::false_type) { return 0; }
			template <class... TsA>
			constexpr static RFlagValue_t CalcAcFlag(CType<TsA...>) { return 0; }
			template <class T, class... TsA>
			constexpr static RFlagValue_t CalcAcFlag(CType<T,TsA...>) {
				return ReturnIf(OrLH<T>(), IsAcWrapper<typename T::value_type>()) | CalcAcFlag(CType<TsA...>());
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
				return _Sum<TsA...>(IConst<sizeof...(TsA)>());
			}
			//! 変数全てを示すフラグ値
			static constexpr RFlagValue_t All() {
				return (1 << (sizeof...(Ts)+1)) -1;
			}
			//! 自分より上の階層のフラグ (Low -> High)
			template <class T>
			static constexpr RFlagValue_t OrLH() {
				// TypeListを巡回、A::Has<T>ならOr<A>()をたす
				return Get<T>() | _IterateLH<T>();
			}
			//! 自分以下の階層のフラグ (High -> Low)
			template <class T>
			static constexpr RFlagValue_t OrHL() {
				return Get<T>() | _IterateHL<T>();
			}
			//! 自分以下の階層のフラグ (High -> Low) 直下のみ
			template <class T>
			static constexpr RFlagValue_t OrHL0() {
				return Get<T>() | _IterateHL0<T>();
			}
			constexpr static RFlagValue_t ACFlag = CalcAcFlag(ct_base());

			//! 累積カウンタ値取得
			template <class TA>
			AcCounter_t getAcCounter() const {
				return _accum[GetFlagIndex<TA>()];
			}
			//! 現在のフラグ値
			RFlagValue_t getFlag() const {
				return _rflag;
			}
			//! フラグのテストだけする
			template <class... TsA>
			RFlagValue_t test() const {
				return getFlag() & Get<TsA...>();
			}
			//! 更新フラグだけを立てる
			/*! 累積カウンタも更新 */
			template <class... TsA>
			void setFlag() {
				_setFlag<TsA...>();
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
			//! 必要に応じて更新をかけつつ、参照を返す
			template <class T>
			cref_type<T> get(const Class* self) const {
				return refresh<T>(self).first;
			}
			//! リフレッシュコールだけをする
			template <class T>
			CRef_p<T> refresh(const Class* self) const {
				// 全ての変数が更新済みなら参照を返す
				if(!(_rflag & Get<T>()))
					return CRef_p<T>(base::cref(_NullPtr<T>()), 0);
				// 更新をかけて返す
				return _refreshSingle<T>(self);
			}
			template <class... TsA>
			using Tuple_p = std::pair<std::tuple<cptr_type<TsA>...>, RFlagValue_t>;
			template <class... TsA>
			Tuple_p<TsA...> getAsTuple(const Class* self) const {
				Tuple_p<TsA...> ret;
				ret.second = _getAsTuple<0>(self, ret.first, ((TsA*)nullptr)...);
				return ret;
			}
			template <class Acc, class... TsA>
			auto _getWithCheck(const Class* self, Acc& acc, CType<TsA...>) const {
				Tuple_p<TsA...> ret;
				ret.second = _getWithCheck<0>(self, acc, ret.first, ((TsA*)nullptr)...);
				return ret;
			}
			template <class Acc>
			auto getWithCheck(const Class* self, Acc& acc) const {
				return _getWithCheck(self, acc, typename Acc::Flag_t());
			}
	};
	template <class Base, class Getter>
	struct AcCheck {};
	template <class... Ts, class Base, class Getter>
	static AcWrapper<Base, Getter, Ts...> AcDetect(AcCheck<Base, Getter>*);
	template <class... Ts, class T>
	static T AcDetect(T*);
	template <class C>
	struct RFlag_Getter {
		using counter_t = C;
		template <class T, class TP>
		counter_t operator()(const T&, TP*) const {
			return 0;
		}
	};

	#define RFLAG(clazz, seq) \
		using RFlag_t = spn::RFlag<clazz, BOOST_PP_SEQ_ENUM(seq)>; \
		RFlag_t _rflag; \
		template <class T, class... Ts> \
		friend class spn::RFlag;
	#define RFLAG_RVALUE_BASE(name, valueT, ...) \
		struct name : spn::CType<__VA_ARGS__> { \
			using value_type = decltype(spn::AcDetect<__VA_ARGS__>((valueT*)nullptr)); };
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
		::spn::RFlagRet _refresh(typename name::value_type&, name*) const;
	#define RFLAG_RVALUE_(name, valueT) \
		::spn::RFlagRet _refresh(typename name::value_type&, name*) const { return {true, 0}; }
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

