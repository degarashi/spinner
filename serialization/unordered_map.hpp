#pragma once
#include <unordered_map>
#include <boost/serialization/split_free.hpp>

namespace boost {
	namespace serialization {
		template<class Archive, class Key, class T, class Hash, class KeyEqual, class Allocator>
		inline void serialize(Archive& ar, std::unordered_map<Key,T,Hash,KeyEqual,Allocator>& um, const unsigned int ver){
			split_free(ar, um, ver);
		}
		template <class Archive, class Key, class T, class Hash, class KeyEqual, class Allocator>
		void load(Archive& ar, std::unordered_map<Key,T,Hash,KeyEqual,Allocator>& um, const unsigned int ver) {
			um.clear();
			size_t nEnt;
			ar & nEnt;

			Key key;
			T value;
			while(nEnt != 0) {
				ar & key & value;
				um.emplace(std::move(key), std::move(value));
				--nEnt;
			}
		}
		template <class Archive, class Key, class T, class Hash, class KeyEqual, class Allocator>
		void save(Archive& ar, const std::unordered_map<Key,T,Hash,KeyEqual,Allocator>& um, const unsigned int ver) {
			size_t nEnt = um.size();
			ar & nEnt;
			for(auto& p : um)
				ar & p.first & p.second;
		}
	}
}
