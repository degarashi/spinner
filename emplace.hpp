#pragma once
#include <utility>

namespace spn {
	//! 連想コンテナにEmplace(既に要素があったら置き換え)
	template <class Map, class Key, class... Args>
	typename Map::iterator EmplaceOrReplace(Map& m, Key&& key, Args&&... args) {
		typename Map::value_type::second_type item(std::forward<Args>(args)...);
		auto itr = m.find(key);
		if(itr == m.end()) {
			return m.emplace(std::forward<Key>(key), std::move(item)).first;
		} else {
			itr->second = std::move(item);
			return itr;
		}
	}
	//! 連想コンテナにEmplace(既に要素があったら何もせず、オブジェクトも作成しない)
	template <class Map, class Key, class... Args>
	std::pair<typename Map::iterator, bool> TryEmplace(Map& m, Key&& key, Args&&... args) {
		auto itr = m.find(key);
		if(itr == m.end())
			return m.emplace(std::forward<Key>(key), typename Map::value_type::second_type(std::forward<Args>(args)...));
		return std::make_pair(itr, false);
	}
}
