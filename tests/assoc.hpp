#pragma once
#ifdef WIN32
	#include <intrin.h>
#endif
#include "../assoc.hpp"

namespace spn {
	namespace test {
		namespace { namespace asv_inner {
			template <class K, class V>
			void _AddRandomKV(const K& key, const V& value) {}
			template <class K, class V, class A, class... Ts>
			void _AddRandomKV(const K& key, const V& value, A& a, Ts&... ts) {
				a.insert(std::make_pair(key, value));
				_AddRandomKV(key, value, ts...);
			}

			template <class V>
			void _AddRandomV(const V& value) {}
			template <class V, class A, class... Ts>
			void _AddRandomV(const V& value, A& a, Ts&... ts) {
				a.insert(value);
				_AddRandomV(value, ts...);
			}

			bool _RemRandom(int idx) { return true; }
			template <class A, class... Ts>
			bool _RemRandom(int idx, A& a, Ts&... ts) {
				if(a.empty())
					return false;
				auto itr = a.cbegin();
				std::advance(itr, idx);
				a.erase(itr);
				return _RemRandom(idx, ts...) && !a.empty();
			}
		} }

		//! 指定された個数のランダム値を追加
		template <class RDF, class... Ts>
		void AddRandomV(int n, RDF& rdF, Ts&... ts) {
			while(--n >= 0)
				asv_inner::_AddRandomV(rdF(), ts...);
		}
		template <class RDF, class V, class C, class... Ts>
		void AddRandom(int n, RDF& rdF, AssocVec<V,C>& a, Ts&... ts) {
			AddRandomV(n, rdF, a, ts...);
		}
		template <class RDF, class... Ts>
		void AddRandomKV(int n, RDF& rdF, Ts&... ts) {
			while(--n >= 0)
				asv_inner::_AddRandomKV(rdF(), rdF(), ts...);
		}
		template <class RDF, class K, class V, class C, class... Ts>
		void AddRandom(int n, RDF& rdF, AssocVecK<K,V,C>& a, Ts&... ts) {
			AddRandomKV(n, rdF, a, ts...);
		}
		//! 指定された個数の要素をランダムに削除
		template <class RDR, class A, class... Ts>
		void RemRandom(int n, RDR& rdR, A& a, Ts&... ts) {
			if(a.empty())
				return;

			while(--n >= 0) {
				int idx = rdR(0, a.size()-1);
				if(!asv_inner::_RemRandom(idx, a, ts...))
					break;
			}
		}
		namespace asv_inner {}

		//! AssocVecの要素がきちんとソートされているか確認
		template <class A>
		void ChkSequence(const A& a, ...) {
			using C = typename A::cmp_type;
			C cmp;
			auto val = a[0];
			for(auto itr=a.cbegin() ; itr!=a.cend() ; itr++) {
				ASSERT_FALSE(cmp(*itr, val));
				val = *itr;
			}
		}
		// (std::pairの場合)
		template <class A>
		void ChkSequence(const A& a, decltype(std::declval<typename A::value_type>().first)* = nullptr) {
			using C = typename A::cmp_type;
			C cmp;
			auto val = a.getKey(0);
			for(auto itr=a.cbegin() ; itr!=a.cend() ; itr++) {
				ASSERT_FALSE(cmp(itr->first, val));
				val = itr->first;
			}
		}
	}
}

