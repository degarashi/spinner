#include "test.hpp"
#include "../rflag.hpp"
#include "../structure/wrapper.hpp"

namespace spn {
	namespace test {
		struct BaseT {
			enum E {
				Base0,
				Base1,
				Num
			};
		};
		struct MiddleT {
			enum E {
				Middle0,
				Middle1,
				Upper,
				Num
			};
		};
		struct ValueT {
			enum E {
				Base0,
				Base1,
				Middle0,
				Middle1,
				Upper,
				Num
			};
		};
		template <class T, class RD>
		T MakeRandomData(RD& rd) {
			return rd.template getUniform<T>();
		}
		struct Counter {
			int value[MiddleT::Num];
			void clear() {
				for(auto& v : value)
					v = 0;
			}
			bool operator == (const Counter& c) const {
				for(size_t i=0 ; i<countof(value) ; i++) {
					if(value[i] != c.value[i])
						return false;
				}
				return true;
			}
		};
		template <class Self, class T>
		using SetFunc = std::function<void (Self&, T&&)>;
		template <class Self, class T>
		using GetFunc = std::function<const T& (const Self&)>;
		template <class Self, class T>
		using RefFunc = std::function<T& (Self&)>;

		template <class V, bool B>
		class RFTest {
			private:
				struct Getter {
					using counter_t = uint32_t;
				};
				using Base0_t = std::conditional_t<B, AcWrapper<Wrapper<V>, Getter>, V>;
				#define SEQ_CACHE \
					((Base0)(Base0_t)) \
					((Base1)(V)) \
					((Middle0)(V)(Base0)) \
					((Middle1)(V)(Base1)) \
					((Upper)(V)(Middle0)(Middle1))
				RFLAG_S(RFTest, SEQ_CACHE)
				mutable Counter _counter;

				const static SetFunc<RFTest, V>	cs_setfunc[];
				const static GetFunc<RFTest, V>	cs_getfunc[];
				const static RefFunc<RFTest, V>	cs_reffunc[];

			public:
				RFLAG_GETMETHOD_S(SEQ_CACHE)
				RFLAG_SETMETHOD_S(SEQ_CACHE)
				RFLAG_REFMETHOD_S(SEQ_CACHE)

				RFLAG_SETMETHOD(Middle0)
				RFLAG_SETMETHOD(Middle1)
				RFLAG_SETMETHOD(Upper)
				RFLAG_REFMETHOD(Middle0)
				RFLAG_REFMETHOD(Middle1)
				RFLAG_REFMETHOD(Upper)

				RFTest() {
					for(auto& f : cs_setfunc)
						f(*this, 0);
					_rflag.resetAll();
					_counter.clear();
				}
				RFlagValue_t getFlag() const {
					return _rflag.getFlag();
				}
				void set(int idx, V&& t) {
					cs_setfunc[idx](*this, std::move(t));
				}
				const V& get(int idx) const {
					return cs_getfunc[idx](*this);
				}
				V& ref(int idx) {
					return cs_reffunc[idx](*this);
				}
				Counter& refCounter() {
					return _counter;
				}
		};
		template <class T, bool B>
		const SetFunc<RFTest<T,B>, T> RFTest<T,B>::cs_setfunc[ValueT::Num] = {
			[](RFTest& self, T&& t) { self.setBase0(std::move(t)); },
			[](RFTest& self, T&& t) { self.setBase1(std::move(t)); },
			[](RFTest& self, T&& t) { self.setMiddle0(std::move(t)); },
			[](RFTest& self, T&& t) { self.setMiddle1(std::move(t)); },
			[](RFTest& self, T&& t) { self.setUpper(std::move(t)); }
		};
		template <class T, bool B>
		const GetFunc<RFTest<T,B>, T> RFTest<T,B>::cs_getfunc[ValueT::Num] = {
			[](const RFTest& self) -> const T& { return self.getBase0(); },
			[](const RFTest& self) -> const T& { return self.getBase1(); },
			[](const RFTest& self) -> const T& { return self.getMiddle0(); },
			[](const RFTest& self) -> const T& { return self.getMiddle1(); },
			[](const RFTest& self) -> const T& { return self.getUpper(); }
		};
		template <class T, bool B>
		const RefFunc<RFTest<T,B>, T> RFTest<T,B>::cs_reffunc[ValueT::Num] = {
			[](RFTest& self) -> T& { return self.refBase0(); },
			[](RFTest& self) -> T& { return self.refBase1(); },
			[](RFTest& self) -> T& { return self.refMiddle0(); },
			[](RFTest& self) -> T& { return self.refMiddle1(); },
			[](RFTest& self) -> T& { return self.refUpper(); }
		};

		template <class T, bool B>
		RFlagRet RFTest<T,B>::_refresh(typename Middle0::value_type& v, Middle0*) const {
			v = getBase0();
			++_counter.value[MiddleT::Middle0];
			return {true, 0};
		}
		template <class T, bool B>
		RFlagRet RFTest<T,B>::_refresh(typename Middle1::value_type& v, Middle1*) const {
			v = getBase1();
			++_counter.value[MiddleT::Middle1];
			return {true, 0};
		}
		template <class T, bool B>
		RFlagRet RFTest<T,B>::_refresh(typename Upper::value_type& v, Upper*) const {
			v = getMiddle0() + getMiddle1();
			++_counter.value[MiddleT::Upper];
			return {true, 0};
		}

		template <class T, bool B>
		class RFCheck {
			private:
				mutable bool			_flag[ValueT::Num];
				mutable T				_value[ValueT::Num];
				mutable Counter			_counter;

				const static SetFunc<RFCheck, T>	cs_setfunc[];
				const static GetFunc<RFCheck, T>	cs_getfunc[];
				const static RefFunc<RFCheck, T>	cs_reffunc[];
			public:
				RFlagValue_t getFlag() const {
					RFlagValue_t	flag = 0,
									cur = 0x01;
					for(auto& b : _flag) {
						if(b)
							flag |= cur;
						cur <<= 1;
					}
					return flag | cur;
				}
				void set(int idx, T&& t) {
					cs_setfunc[idx](*this, std::move(t));
				}
				const T& get(int idx) const {
					return cs_getfunc[idx](*this);
				}
				T& ref(int idx) {
					return cs_reffunc[idx](*this);
				}
				RFCheck() {
					for(auto& f : _flag)
						f = true;
					for(auto& v : _value)
						v = 0;
					_counter.clear();
				}
				Counter& refCounter() {
					return _counter;
				}
				void acflag_set() const {
					if(B) {
						_flag[ValueT::Base0] =
						_flag[ValueT::Middle0] =
						_flag[ValueT::Upper] = true;
					}
				}
		};
		template <class T, bool B>
		const SetFunc<RFCheck<T,B>, T> RFCheck<T,B>::cs_setfunc[ValueT::Num] = {
			[](RFCheck& self, T&& t){
				auto& v = self._value;
				auto& f = self._flag;
				v[ValueT::Base0] = std::move(t);
				f[ValueT::Base0] = false;

				f[ValueT::Middle0] = true;
				f[ValueT::Upper] = true;
				self.acflag_set();
			},
			[](RFCheck& self, T&& t){
				auto& v = self._value;
				auto& f = self._flag;
				v[ValueT::Base1] = std::move(t);
				f[ValueT::Base1] = false;

				f[ValueT::Middle1] = true;
				f[ValueT::Upper] = true;
				self.acflag_set();
			},
			[](RFCheck& self, T&& t){
				auto& v = self._value;
				auto& f = self._flag;
				v[ValueT::Middle0] = std::move(t);
				f[ValueT::Middle0] = false;

				f[ValueT::Upper] = true;
				self.acflag_set();
			},
			[](RFCheck& self, T&& t){
				auto& v = self._value;
				auto& f = self._flag;
				v[ValueT::Middle1] = std::move(t);
				f[ValueT::Middle1] = false;

				f[ValueT::Upper] = true;
				self.acflag_set();
			},
			[](RFCheck& self, T&& t){
				auto& v = self._value;
				auto& f = self._flag;
				v[ValueT::Upper] = std::move(t);
				f[ValueT::Upper] = false;
				self.acflag_set();
			},
		};
		bool FChClear(bool& b) {
			bool tmp = b;
			b = false;
			return tmp;
		}
		template <class T, bool B>
		const GetFunc<RFCheck<T,B>, T> RFCheck<T,B>::cs_getfunc[ValueT::Num] = {
			[](const RFCheck& self) -> const T& {
				self.acflag_set();
				return self._value[ValueT::Base0];
			},
			[](const RFCheck& self) -> const T& {
				self.acflag_set();
				return self._value[ValueT::Base1];
			},
			[](const RFCheck& self) -> const T& {
				auto& v = self._value;
				auto& f = self._flag;
				if(FChClear(f[ValueT::Middle0])) {
					++self._counter.value[MiddleT::Middle0];
					v[ValueT::Middle0] = cs_getfunc[ValueT::Base0](self);
				}
				self.acflag_set();
				return v[ValueT::Middle0];
			},
			[](const RFCheck& self) -> const T& {
				auto& v = self._value;
				auto& f = self._flag;
				if(FChClear(f[ValueT::Middle1])) {
					++self._counter.value[MiddleT::Middle1];
					v[ValueT::Middle1] = cs_getfunc[ValueT::Base1](self);
				}
				self.acflag_set();
				return v[ValueT::Middle1];
			},
			[](const RFCheck& self) -> const T& {
				auto& v = self._value;
				auto& f = self._flag;
				if(FChClear(f[ValueT::Upper])) {
					++self._counter.value[MiddleT::Upper];
					v[ValueT::Upper] = cs_getfunc[ValueT::Middle0](self) + cs_getfunc[ValueT::Middle1](self);
				}
				self.acflag_set();
				return v[ValueT::Upper];
			}
		};
		#define DEF_REFFUNC(name) [](RFCheck<T,B>& self) -> T& { \
				T tmp(std::move(self._value[ValueT::name])); \
				cs_setfunc[ValueT::name](self, std::move(tmp)); \
				return self._value[ValueT::name]; \
			},
		template <class T, bool B>
		const RefFunc<RFCheck<T,B>, T> RFCheck<T,B>::cs_reffunc[ValueT::Num] = {
			DEF_REFFUNC(Base0)
			DEF_REFFUNC(Base1)
			DEF_REFFUNC(Middle0)
			DEF_REFFUNC(Middle1)
			DEF_REFFUNC(Upper)
		};
		#undef DEF_REFFUNC

		template <class T>
		class RefreshFlagTest : public RandomTestInitializer {};
		using FlagT = ::testing::Types<std::pair<int32_t, std::false_type>,
										std::pair<int64_t, std::false_type>,
										std::pair<int32_t, std::true_type>,
										std::pair<int64_t, std::true_type>>;
		TYPED_TEST_CASE(RefreshFlagTest, FlagT);
		TYPED_TEST(RefreshFlagTest, General) {
			auto rd = this->getRand();
			using Type = typename TypeParam::first_type;
			using RF_t = RFTest<Type, TypeParam::second_type::value>;
			using RFChk_t = RFCheck<Type, TypeParam::second_type::value>;
			RF_t	test;
			RFChk_t chk;

			struct OP {
				enum E {
					Set,
					Get,
					Ref,
					Num
				};
			};
			auto IRand = rd.template getUniformF<int>();
			// ランダムに値をset, get, refして値や更新カウンタが想定と一致しているかチェック
			int nOp = IRand({0,32});
			while(nOp-- > 0) {
				const int idx = IRand({0, ValueT::Num-1});
				switch(IRand({0, OP::Num-1})) {
					case OP::Set: {
						// set
						auto data = MakeRandomData<Type>(rd);
						test.set(idx, Type(data));
						chk.set(idx, Type(data));
						break; }
					case OP::Get: {
						// get
						auto& data0 = test.get(idx);
						auto& data1 = chk.get(idx);
						// 値の比較
						ASSERT_EQ(data0, data1);
						break; }
					case OP::Ref: {
						// ref
						auto& data0 = test.ref(idx);
						auto& data1 = chk.ref(idx);
						// 値の比較
						ASSERT_EQ(data0, data1);
						auto data = MakeRandomData<Type>(rd);
						data0 = Type(data);
						data1 = Type(data);
						break; }
				}
			}
			// 値比較
			for(int i=0 ; i<ValueT::Num ; i++) {
				ASSERT_EQ(chk.get(i), test.get(i));
			}
			// 更新カウンタ値比較
			ASSERT_EQ(chk.refCounter(), test.refCounter());
		}
	}
}
