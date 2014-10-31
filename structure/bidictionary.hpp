#pragma once
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unique_ptr.hpp>

namespace spn {
	struct none_t;

	namespace {
		namespace bidic_in {
			template <class TTo, class TFrom>
			auto _ChangeAllocator(std::allocator<TFrom>* = nullptr) -> std::allocator<TTo>;
			template <class TTo, class T>
			auto _ChangeAllocator(T* = nullptr) -> T;
			template <template <class...> class T, class... Ts, class TTo>
			auto ChangeAllocator(T<Ts...>*, TTo*) -> T<decltype(_ChangeAllocator<TTo>((Ts*)nullptr))...>;

			// decltype(GetT<Ts>(nullptr))...
			// firstとallocatorだけ型をすり替え
			template <template <class...> class T, class T0, class T1, class... Ts, class TTo>
			auto ChangeSecondType(T<T0, T1, Ts...>*, TTo*) -> T<T0, TTo, Ts...>;

			template <class T, class B>
			struct ValueHolder;
			template <class T>
			struct ValueHolder<T, std::true_type> {
				T*		value;
				bool	_bDelete;

				template <class Ar>
				void serialize(Ar& ar, const unsigned int) {
					ar	& BOOST_SERIALIZATION_NVP(value);
				}
				// Load
				ValueHolder():
					value(nullptr),
					_bDelete(true)
				{}
				// Save
				ValueHolder(const T& src):
					value(const_cast<T*>(&src)),
					_bDelete(false)
				{}
				ValueHolder(const ValueHolder& h):
					value(h.value),
					_bDelete(h._bDelete)
				{
					auto& h2 = const_cast<ValueHolder&>(h);
					h2.value = nullptr;
					h2._bDelete = false;
				}

				~ValueHolder() {
					if(_bDelete && value)
						delete value;
				}
				T take() {
					return std::move(*value);
				}
			};
			template <class T>
			struct ValueHolder<T, std::false_type> {
				T	value;

				template <class Ar>
				void serialize(Ar& ar, const unsigned int) {
					ar	& BOOST_SERIALIZATION_NVP(value);
				}
				// Load
				ValueHolder():
					value(0) {}
				// Save
				ValueHolder(const T& src):
					value(src) {}
				T take() {
					return value;
				}
			};
		}
	}

	template <class MapT0, class MapT1, class Value=spn::none_t>
	class BiDictionary {
		private:
			using Key0 = typename MapT0::key_type;
			using Key1 = typename MapT1::key_type;
			struct Entry {
				Value			value;
				const Key0*		key0;
				const Key1*		key1;

				Entry() = default;
				Entry(Value&& v): value(std::move(v)) {}
			};
			using In0 = decltype(bidic_in::ChangeSecondType((MapT0*)nullptr, (Entry**)nullptr));
			using In1 = decltype(bidic_in::ChangeSecondType((MapT1*)nullptr, (Entry**)nullptr));
			using Map0 = decltype(bidic_in::ChangeAllocator((In0*)nullptr, (std::pair<const Key0, Value>*)nullptr));
			using Map1 = decltype(bidic_in::ChangeAllocator((In1*)nullptr, (std::pair<const Key1, Value>*)nullptr));
			Map0	_map0;
			Map1	_map1;

			using Itr0 = typename Map0::iterator;
			using ItrC0 = typename Map0::const_iterator;
			using Itr1 = typename Map1::iterator;
			using ItrC1 = typename Map1::const_iterator;
			using Val0 = typename Map0::mapped_type;
			using Val1 = typename Map1::mapped_type;

			friend class boost::serialization::access;
			BOOST_SERIALIZATION_SPLIT_MEMBER();
			template <class Ar>
			void save(Ar& ar, const unsigned int) const {
				std::vector<EntrySave>	entry;
				for(auto& p : _map0)
					entry.emplace_back(p.second);
				ar	& BOOST_SERIALIZATION_NVP(entry);
			}
			template <class Ar>
			void load(Ar& ar, const unsigned int) {
				_map0.clear();
				_map1.clear();

				std::vector<EntryLoad>	entry;
				ar	& BOOST_SERIALIZATION_NVP(entry);
				for(auto& p : entry)
					insert(p.key0, p.key1, p.value.take());
			}
			using ValueHolder_t = bidic_in::ValueHolder<Value, typename std::is_class<Value>::type>;
			struct EntrySave {
				ValueHolder_t		value;
				const Key0&			key0;
				const Key1&			key1;

				BOOST_SERIALIZATION_SPLIT_MEMBER();
				template <class Ar>
				void save(Ar& ar, const unsigned int) const {
					ar	& BOOST_SERIALIZATION_NVP(value)
						& BOOST_SERIALIZATION_NVP(key0)
						& BOOST_SERIALIZATION_NVP(key1);
				}
				template <class Ar>
				void load(Ar& ar, const unsigned int) {}

				EntrySave(const Entry* e):
					value(e->value),
					key0(*e->key0),
					key1(*e->key1) {}
			};
			struct EntryLoad {
				ValueHolder_t		value;
				Key0				key0;
				Key1				key1;

				BOOST_SERIALIZATION_SPLIT_MEMBER();
				template <class Ar>
				void save(Ar& ar, const unsigned int) const {}
				template <class Ar>
				void load(Ar& ar, const unsigned int) {
					ar	& BOOST_SERIALIZATION_NVP(value)
						& BOOST_SERIALIZATION_NVP(key0)
						& BOOST_SERIALIZATION_NVP(key1);
				}
			};

		public:
			using key_type0 = Key0;
			using key_type1 = Key1;
			using value_type = Value;
		public:
			~BiDictionary() {
				clear();
			}
			void clear() {
				for(auto& p : _map0)
					delete reinterpret_cast<Entry*>(p.second);
				_map0.clear();
				_map1.clear();
			}
			bool insert(const Key0& key0, const Key1& key1, Value&& v) {
				auto* ent = new Entry(std::move(v));
				auto res0 = _map0.emplace(key0, ent);
				if(!res0.second)
					return false;

				auto res1 = _map1.emplace(key1, ent);
				if(!res1.second) {
					_map0.erase(res0.first);
					return false;
				}
				ent->key0 = &res0.first->first;
				ent->key1 = &res1.first->first;
				return true;
			}
			bool insert(const Key0& key0, const Key1& key1, const Value& v) {
				return insert(key0, key1, Value(v));
			}
			template <class... Args>
			bool emplace(const Key0& key0, const Key1& key1, Args&&... args) {
				return insert(key0, key1, Value(std::forward<Args>(args)...));
			}

			BiDictionary& operator = (BiDictionary&& b) {
				_map0 = std::move(b._map0);
				_map1 = std::move(b._map1);
				return *this;
			}
			BiDictionary& operator = (const BiDictionary& b) {
				clear();
				for(auto& p : b._map0)
					insert(*p.key0, *p.key1, p.value);
				return *this;
			}
			bool empty() const {
				return _map0.empty();
			}
			size_t size() const {
				return _map0.size();
			}
			bool operator == (const BiDictionary& b) const {
				if(_map0.size() != b._map0.size())
					return false;
				for(auto& p : _map0) {
					auto itr = b._map0.find(p.first);
					if(itr == b._map0.end())
						return false;
					if(p.second->value != itr->second->value)
						return false;
				}
				return true;
			}
			bool operator != (const BiDictionary& b) const {
				return !(this->operator ==(b));
			}

			std::pair<Itr0, Itr1> _erase(Entry* e) {
				auto itr0 = _map0.erase(_map0.find(*e->key0));
				auto itr1 = _map1.erase(_map1.find(*e->key1));
				delete e;
				return std::make_pair(itr0, itr1);
			}
			template <class I>
			I erase(I itrB, I itrE) {
				while(itrB != itrE)
					itrB = erase(itrB);
				return itrB;
			}
			Itr0 erase(Itr0 itr0) {
				return _erase(itr0->second).first;
			}
			Itr1 erase(Itr1 itr1) {
				return _erase(itr1->second).second;
			}

			// 元クラスのメソッドのアダプタ
			#define ADAPTER_BASE(method, ...)	\
				template <class... Ts> \
				auto method(Ts&&... ts) __VA_ARGS__ -> decltype(_self.method(std::forward<Ts>(ts)...)) { return _self.method(std::forward<Ts>(ts)...); }
			#define ADAPTER(method) \
				ADAPTER_BASE(method)
			#define ADAPTER_C(method) \
				ADAPTER_BASE(method, const)
			#define ADAPTER_C2(method) \
				ADAPTER_BASE(method) \
				ADAPTER_BASE(method, const)
			// 指定マップクラスのアダプタ
			#define NA_ADAPTER_BASE(method, ...) \
				auto method() __VA_ARGS__ -> decltype(_map.method()) { return _map.method(); }
			#define NA_ADAPTER(method) \
				NA_ADAPTER_BASE(method)
			#define NA_ADAPTER_C(method) \
				NA_ADAPTER_BASE(method, const)
			#define NA_ADAPTER_C2(method) \
				NA_ADAPTER_BASE(method) \
				NA_ADAPTER_BASE(method, const)

			#define DEF_METHODS(num) \
				Itr##num begin_##num() { return _map##num.begin(); } \
				Itr##num end_##num() { return _map##num.end(); } \
				ItrC##num begin_##num() const { return _map##num.begin(); } \
				ItrC##num end_##num() const { return _map##num.end(); } \
				ItrC##num cbegin_##num() const { return _map##num.cbegin(); } \
				ItrC##num cend_##num() const { return _map##num.cend(); } \
				Itr##num find(const Key##num& k) { return _map##num.find(k); } \
				ItrC##num find(const Key##num& k) const { return _map##num.find(k); } \
				const Val##num& at(const Key##num& k) const { return _map##num.at(k); } \
				Val##num& at(const Key##num& k) { return _map##num.at(k); } \
				Val##num& operator [](const Key##num& k) { return _map##num[k]; } \
				size_t count(const Key##num& k) const { return _map##num.count(k); } \
				class Inner##num { \
					private: \
						using Map = Map##num; \
						BiDictionary&	_self; \
						Map&			_map; \
					public: \
						Inner##num(BiDictionary& self): _self(self), _map(self._map##num) {} \
						Inner##num(Inner##num&& n) = default; \
						NA_ADAPTER(begin) \
						NA_ADAPTER_C2(cbegin) \
						NA_ADAPTER(end) \
						NA_ADAPTER_C2(cend) \
						operator const Map&() const { \
							return _map; \
						} \
						bool operator == (const Map& a) const { \
							return _map == a; \
						} \
						bool operator != (const Map& a) const { \
							return !(this->operator ==(a)); \
						} \
						ADAPTER(empty) \
						ADAPTER(size) \
						ADAPTER(clear) \
						ADAPTER_C2(at) \
						ADAPTER(operator[]) \
						ADAPTER_C(count) \
						ADAPTER_C2(find) \
						ADAPTER(erase) \
				}; \
				Inner##num _refMap(Key##num*) { \
					return Inner##num(*this); \
				}
			template <class K>
			auto refMap() {
				return _refMap((K*)nullptr);
			}

			DEF_METHODS(0)
			DEF_METHODS(1)
			#undef DEF_METHODS
			#undef ADAPTER_BASE
			#undef ADAPTER_C
			#undef ADAPTER_C2
			#undef NA_ADAPTER_BASE
			#undef NA_ADAPTER_C
			#undef NA_ADAPTER_C2
	};
	namespace bidic_in {}
}
