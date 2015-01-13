#pragma once
#include <algorithm>
#include <utility>
#include <iterator>

namespace spn {
	//! 単純挿入ソート
	template <class Itr, class Cmp>
	void insertion_sort(Itr itrB, Itr itrE, const Cmp& cmp) {
		if(std::distance(itrB, itrE) < 2)
			return;

		// ソート済のカーソル
		Itr curLE = std::next(itrB);
		for(;;) {
			Itr curL1 = curLE,
				curL0 = std::prev(curL1);
			auto tmp = std::move(*curL1);
			while(!cmp(*curL0, tmp)) {
				*curL1 = std::move(*curL0);
				--curL1;
				if(curL0 == itrB)
					break;
				--curL0;
			}
			*curL1 = std::move(tmp);
			// ソート済みカーソルがリストの最後まで達したら終了
			if(++curLE == itrE)
				break;
		}
	}
	//! 単純挿入ソート (リスト用)
	template <class List, class Cmp>
	void insertion_sort_list(List& l, typename List::iterator itrB, typename List::iterator itrE, const Cmp& cmp) {
		if(std::distance(itrB, itrE) < 2)
			return;

		auto itrLE = std::next(itrB);
		for(;;) {
			auto itrL = itrLE;
			do {
				--itrL;
				if(cmp(*itrL, *itrLE)) {
					++itrL;
					break;
				}
			} while(itrL != itrB);

			if(itrL != itrLE) {
				auto tmp = std::move(*itrLE);
				itrLE = l.erase(itrLE);
				auto itr = l.insert(itrL, std::move(tmp));
				if(itrL == itrB)
					itrB = itr;
			} else
				++itrLE;
			if(itrLE == itrE)
				break;
		}
	}
}

