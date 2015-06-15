#ifdef WIN32
	#include <intrin.h>
#endif
#include "test.hpp"
#include "../structure/bidictionary.hpp"
#include <unordered_map>
#include <map>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "../random/string.hpp"

namespace spn {
	namespace test {
		// 検証用のデータ構造
		template <class Key0, class Key1, class Value>
		class BiDicCheck {
			private:
				using ValueSP = std::shared_ptr<Value>;
				using Map0 = std::unordered_map<Key0, ValueSP>;
				using Map1 = std::unordered_map<Key1, ValueSP>;
				Map0	_map0;
				Map1	_map1;
			public:
				#define METHODS(num) \
					using Itr##num = typename Map##num::iterator; \
					using ItrC##num = typename Map##num::const_iterator; \
					Itr##num begin_##num() { return _map##num.begin(); } \
					Itr##num end_##num() { return _map##num.end(); } \
					ItrC##num begin_##num() const { return _map##num.begin(); } \
					ItrC##num end_##num() const { return _map##num.end(); } \
					ItrC##num cbegin_##num() const { return _map##num.cbegin(); } \
					ItrC##num cend_##num() const { return _map##num.cend(); } \
					Itr##num erase(Itr##num itr) { return _map##num.erase(itr); }
				METHODS(0)
				METHODS(1)
				#undef METHODS

				void clear() {
					_map0.clear();
					_map1.clear();
				}
				void insert(const Key0& k0, const Key1& k1, Value&& v) {
					auto spValue = std::make_shared<Value>(std::move(v));
					_map0.emplace(k0, spValue);
					_map1.emplace(k1, spValue);
				}
				void insert(const Key0& k0, const Key1& k1, const Value& v) {
					insert(k0, k1, Value(v));
				}
				template <class... Ts>
				void emplace(const Key0& k0, const Key1& k1, Ts&&... ts) {
					insert(k0, k1, Value(std::forward<Ts>(ts)...));
				}
				BiDicCheck& operator = (const BiDicCheck&) = default;
				BiDicCheck& operator = (BiDicCheck&&) = default;

				bool empty() const {
					return _map0.empty();
				}
				size_t size() const {
					return _map0.size();
				}
				bool operator == (const BiDicCheck& dc) const {
					return _map0 == dc._map0;
				}
				bool operator != (const BiDicCheck& dc) const {
					return _map0 != dc._map0;
				}
		};
		namespace {
			// 本来はコピー不可のクラスをテスト用にmemcpyでコピー
			template <class T>
			T CloneIt(const T& t) {
				char buff[sizeof(T)];
				auto& ref = reinterpret_cast<T&>(buff);
				std::memcpy(buff, &t, sizeof(T));
				return std::move(ref);
			}
			// 構造体にランダムな値を追加
			template <class E>
			void _AddRandom(const E&) {}
			template <class E, class T0, class... Ts>
			void _AddRandom(const E& ent, T0& t, Ts&... ts) {
				t.emplace(ent.key0, ent.key1, CloneIt(ent.value));
				_AddRandom(ent, ts...);
			}
			template <class RD, class... Ts>
			void AddRandom(int n, RD&& rdValue, Ts&... ts) {
				while(--n >= 0)
					_AddRandom(rdValue(), ts...);
			}
			// 構造体からランダムに値を取り除く
			void _RemRandom(int) {}
			template <class T0, class... Ts>
			void _RemRandom(int idx, T0& t, Ts&... ts) {
				auto itr = t.begin_0();
				std::advance(itr, idx);
				t.erase(itr);
				_RemRandom(idx, ts...);
			}
			template <class RD, class T0, class... Ts>
			bool RemRandom(int n, RD&& rdR, T0& t, Ts&... ts) {
				while(--n >= 0) {
					if(t.empty())
						return false;
					_RemRandom(rdR(0,t.size()-1), t, ts...);
				}
				return true;
			}
			// 構造体の比較
			template <class T0, class T1>
			void FindCheck(const T0& t0, const T1& t1) {
				for(auto itr0=t0.begin_0() ; itr0!=t0.end_0() ; ++itr0) {
					auto itr1 = t1.find(itr0->first);
					ASSERT_NE(itr1, t1.end_0());
					ASSERT_EQ(*itr0->second, itr1->second->value);
				}
			}
			// ランダムなキー & 値生成
			template <class RD>
			std::string GenRandomValue(RD& rd, std::string*) {
				return random::GenRAlphabet(rd, 32);
			}
			template <class RD, class T,
					 class=std::enable_if_t<std::is_arithmetic<T>::value>>
			T GenRandomValue(RD& rd, T*) {
				return rd.template getUniform<T>();
			}
			template <class RD, class T>
			MoveOnly<T> GenRandomValue(RD& rd, MoveOnly<T>*) {
				return MoveOnly<T>(rd.template getUniform<T>());
			}
		}

		template <class T>
		class BiDictionaryTest : public RandomTestInitializer {};
		using BiDicT = ::testing::Types<
							std::tuple<int, std::string, double>,
							std::tuple<std::string, int, double>,
							std::tuple<int, std::string, MoveOnly<int>>,
							std::tuple<std::string, int, MoveOnly<int>>
						>;
		TYPED_TEST_CASE(BiDictionaryTest, BiDicT);
		template <class K0, class K1, class V>
		struct Entry {
			K0	key0;
			K1	key1;
			V	value;
			Entry(Entry&&) = default;
			Entry(const K0& k0, const K1& k1, V&& v):
				key0(k0), key1(k1), value(std::move(v)) {}
		};

		TYPED_TEST(BiDictionaryTest, Elements) {
			using KeyT0 = typename std::tuple_element<0, TypeParam>::type;
			using KeyT1 = typename std::tuple_element<1, TypeParam>::type;
			using ValueT = typename std::tuple_element<2, TypeParam>::type;
			using BChk = BiDicCheck<KeyT0, KeyT1, ValueT>;
			using BDic = BiDictionary<std::unordered_map<KeyT0, void*>,
									std::map<KeyT1, void*>,
									ValueT>;
			BChk	bchk;
			BDic	bdic;
			using Ent = Entry<KeyT0, KeyT1, ValueT>;
			auto rd = this->getRand();
			auto rdEnt = [&rd](){
				return Ent{GenRandomValue(rd, (KeyT0*)nullptr),
					GenRandomValue(rd, (KeyT1*)nullptr),
					GenRandomValue(rd, (ValueT*)nullptr)};
			};
			auto rdR = [&rd](int from, int to){ return rd.template getUniform<int>({from,to}); };
			// ランダムなデータを追加
			AddRandom(rdR(1, 0xf00), rdEnt, bchk, bdic);
			// ランダムにデータを削除
			RemRandom(rdR(0, 0x100), rdR, bchk, bdic);
			// データ検索(検証用のデータと比較)
			ASSERT_NO_FATAL_FAILURE(FindCheck(bchk, bdic));
		}
		TYPED_TEST(BiDictionaryTest, Serialize) {
			using KeyT0 = typename std::tuple_element<0, TypeParam>::type;
			using KeyT1 = typename std::tuple_element<1, TypeParam>::type;
			using ValueT = typename std::tuple_element<2, TypeParam>::type;
			using BDic = BiDictionary<std::unordered_map<KeyT0, void*>,
									std::map<KeyT1, void*>,
									ValueT>;
			using Ent = Entry<KeyT0, KeyT1, ValueT>;
			auto rd = this->getRand();
			auto rdR = [&rd](int from, int to){ return rd.template getUniform<int>({from,to}); };
			auto rdEnt = [&rd](){
				return Ent{GenRandomValue(rd, (KeyT0*)nullptr),
					GenRandomValue(rd, (KeyT1*)nullptr),
					GenRandomValue(rd, (ValueT*)nullptr)};
			};
			BDic	bdic;
			AddRandom(rdR(0, 0xf00), rdEnt, bdic);
			CheckSerializedDataBin(bdic);
		}
	}
}

