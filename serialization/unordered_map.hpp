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
		void load(Archive& ar, std::unordered_map<Key,T,Hash,KeyEqual,Allocator>& um, const unsigned int /*ver*/) {
			um.clear();

			// 一度pointer listに変換
			std::vector<std::pair<Key,T>*> ptr;
			ar & boost::serialization::make_nvp("data", ptr);
			for(auto* p : ptr)
				um.emplace(p->first, std::move(p->second));
		}
		template <class Archive, class Key, class T, class Hash, class KeyEqual, class Allocator>
		void save(Archive& ar, const std::unordered_map<Key,T,Hash,KeyEqual,Allocator>& um, const unsigned int /*ver*/) {
			// 一度pointer listに変換
			std::vector<std::pair<Key,T>*> ptr;
			for(auto& p : um)
				ptr.push_back(&p);
			ar & boost::serialization::make_nvp("data", ptr);
		}
	}
}
