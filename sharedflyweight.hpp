#pragma once
#include <unordered_map>
#include "error.hpp"

namespace spn {
	template <class T>
	struct Chk_SharedPtr {
		using sp_t = std::shared_ptr<T>;
		using wp_t = std::weak_ptr<T>;
		static std::size_t Count(const wp_t& t) {
			return t.use_count();
		}
		static auto Lock(const wp_t& t) {
			return t.lock();
		}
	};
	template <class K, class V, class Gen, class Chk>
	class SharedFlyweight {
		private:
			using Map = std::unordered_map<K, typename Chk::wp_t>;
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
				return Chk::Count(itr->second);
			}
			auto get(const K& key) {
				auto itr = _map.find(key);
				if(itr == _map.end()) {
					typename Chk::sp_t ret = _generator(key);
					itr = _map.emplace(key, typename Chk::wp_t(ret)).first;
					return ret;
				}
				return Chk::Lock(itr->second);
			}
			void collectGarbage() const {
				auto& m = const_cast<Map&>(_map);
				auto itr = m.begin();
				while(itr != m.end()) {
					if(Chk::Count(itr->second)==0) {
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
