#pragma once
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/unordered_set.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include "check_macro.hpp"

namespace spn {
	DEF_HASMETHOD_T(emplace_back)

	template <class MAP, class VEC=std::vector<
							typename MAP::value_type*,
							typename MAP::allocator_type::template rebind<
												typename MAP::value_type*
							>::other
	>>
	class MapVec {
		private:
			MAP		_map;
			VEC		_vec;

			friend class boost::serialization::access;
			template<class Ar>
			void serialize(Ar& ar, const unsigned int) {
				ar	& BOOST_SERIALIZATION_NVP(_map);
				if(typename Ar::is_loading()) {
					_vec.clear();
					_syncMapToVec();
				}
			}
			class SyncMap {
				private:
					MapVec&	_self;
				public:
					SyncMap(MapVec& self): _self(self) {}
					SyncMap(SyncMap&&) = default;
					~SyncMap() { _self._syncMapToVec(); }
					MAP* operator -> () const { return &_self._map; }
					MAP& operator * () const { return _self._map; }
			};
			class SyncVec {
				private:
					MapVec&	_self;
				public:
					SyncVec(MapVec& self): _self(self) {}
					SyncVec(SyncVec&&) = default;
					~SyncVec() { _self._syncVecToMap(); }
					VEC* operator -> () const { return &_self._vec; }
					VEC& operator * () const { return _self._vec; }
			};

			template <class A>
			void _addVec(A&& a, std::true_type) {
				_vec.emplace_back(std::forward<A>(a));
			}
			template <class A>
			void _addVec(A&& a, std::false_type) {
				_vec.emplace(std::forward<A>(a));
			}
			template <class P>
			static auto _GetFirst(P& p, ...) -> P& {
				return p;
			}
			template <class P>
			static auto _GetFirst(P& p, decltype(p.first)* ptr=nullptr) -> decltype(p.first)& {
				return p.first;
			}

			//! Map優先Sync
			void _syncMapToVec() {
				// Mapに登録されていないVec要素を削除
				int ns = _vec.size();
				for(int i=0 ; i<ns ;) {
					if(_map.count(_GetFirst(*_vec[i], nullptr)) == 0) {
						auto itr = _vec.cbegin();
						std::advance(itr, i);
						_vec.erase(itr);
						--ns;
						continue;
					}
					++i;
				}
				// Vecに登録されていないMap要素を登録
				for(auto& p : _map) {
					auto itr = std::find(_vec.cbegin(), _vec.cend(), &p);
					if(itr == _vec.cend()) {
						// emplace_back()を持っていればそれを、無ければemplace()を呼ぶ
						_addVec(&p, HasMethod_emplace_back<VEC>(nullptr));
					}
				}
			}
			//! Vec優先Sync
			void _syncVecToMap() {
				if(_map.size() != _vec.size()) {
					// Vecに登録されていないMap要素を削除
					for(auto itr=_map.begin() ; itr!=_map.end() ;) {
						auto& p = *itr;
						if(std::find(_vec.cbegin(), _vec.cend(), &p) == _vec.cend())
							itr = _map.erase(itr);
						else
							++itr;
					}
				}
			}
		public:
			bool operator == (const MapVec& mv) const {
				return _map == mv._map;
			}
			bool operator != (const MapVec& mv) const {
				return !(this->operator == (mv));
			}
			size_t size() const {
				return _map.size();
			}
			void clear() {
				_map.clear();
				_vec.clear();
			}
			typename MAP::iterator find(const typename MAP::key_type& val) {
				return _map.find(val);
			}
			typename MAP::const_iterator find(const typename MAP::key_type& val) const {
				return _map.find(val);
			}
			const MAP& getMap() const {
				return _map;
			}
			const VEC& getVec() const {
				return _vec;
			}
			SyncMap refMap() {
				return SyncMap(*this);
			}
			SyncVec refVec() {
				return SyncVec(*this);
			}
	};
}

