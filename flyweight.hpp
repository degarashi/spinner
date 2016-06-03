#pragma once
#include <unordered_map>
#include "error.hpp"

namespace spn {
	template <class K, class V, class Gen>
	class Flyweight {
		private:
			struct Data {
				V		value;
				int		count;

				Data(Data&&) = default;
				template <class V2>
				Data(V2&& v2):
					value(std::forward<V2>(v2)),
					count(0)
				{}
			};
			using Map = std::unordered_map<K, Data>;
			Map		_map;
			struct RHashCmp {
				auto operator()(const V* k) const {
					return std::hash<V>()(*k);
				}
				bool operator()(const V* k0, const V* k1) const {
					return *k0 == *k1;
				}
			};
			using RMap = std::unordered_map<const V*, K, RHashCmp, RHashCmp>;
			RMap	_rmap;
			Gen		_generator;
		public:
			template <class G>
			Flyweight(G&& g):
				_generator(std::forward<G>(g))
			{}
			bool has(const K& key) const {
				return count(key) > 0;
			}
			int count(const K& key) const {
				const auto itr = _map.find(key);
				if(itr == _map.end())
					return 0;
				return itr->second.count;
			}
			const V& get(const K& key) {
				auto itr = _map.find(key);
				if(itr == _map.end()) {
					itr = _map.emplace(key, _generator(key)).first;
					AssertP(Trap, _rmap.count(&itr->second.value)==0)
					_rmap.emplace(&itr->second.value, key);
				}
				++itr->second.count;
				return itr->second.value;
			}
			void putKey(const K& key) {
				const auto itr = _map.find(key);
				AssertT(Throw, itr != _map.end(), std::invalid_argument, "key not found")
				putValue(itr->second.value);
			}
			void putValue(const V& value) {
				const auto ritr = _rmap.find(&value);
				AssertT(Throw, ritr != _rmap.end(), std::invalid_argument, "value not found")
				const auto itr = _map.find(ritr->second);
				AssertP(Trap, itr != _map.end())
				if(--itr->second.count == 0) {
					_rmap.erase(ritr);
					_map.erase(itr);
				}
			}
			void clear() {
				AssertP(Trap, _map.size() == _rmap.size())
				_map.clear();
				_rmap.clear();
			}
			auto size() const {
				return _map.size();
			}
	};
}
