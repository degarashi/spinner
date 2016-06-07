#pragma once
#include <unordered_map>
#include "error.hpp"

namespace spn {
	template <class K, class V, class Gen>
	class SharedFlyweight {
		private:
			using sp_t = std::shared_ptr<V>;
			using wp_t = std::weak_ptr<V>;
			using Map = std::unordered_map<K, wp_t>;
			Map		_map;
			Gen		_generator;
		public:
			template <class G>
			SharedFlyweight(G&& g):
				_generator(std::forward<G>(g))
			{}
			bool has(const K& key) const {
				return count(key) > 0;
			}
			int count(const K& key) const {
				const auto itr = _map.find(key);
				if(itr == _map.end())
					return 0;
				return itr->second.use_count();
			}
			sp_t get(const K& key) {
				auto itr = _map.find(key);
				if(itr == _map.end()) {
					sp_t ret = _generator(key);
					itr = _map.emplace(key, wp_t(ret)).first;
					return ret;
				}
				return itr->second.lock();
			}
			void collectGarbage() const {
				auto& m = const_cast<Map&>(_map);
				auto itr = m.begin();
				while(itr != m.end()) {
					if(itr->second.expired()) {
						const auto itr2 = itr;
						++itr;
						m.erase(itr2);
					} else
						++itr;
				}
			}
			void clear() {
				_map.clear();
			}
			auto size() const {
				collectGarbage();
				return _map.size();
			}
	};
}
