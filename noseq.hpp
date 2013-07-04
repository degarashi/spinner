#pragma once
#include <vector>
#include <type_traits>
#include <algorithm>
#include "type.hpp"
#define DEBUG
namespace spn {
	template <class T>
	class IDPair {
		// Tのサイズが32bit以下ならT, それより上ならuint32_tを選択
		using Index = typename SelectType<TValue<sizeof(T), 32>::greater, uint32_t, T>::type;
		union ItemU {
			struct Item {
				T		value;

				#ifdef DEBUG
					bool	flag;
					Item(): flag(false) {}
					Item(T t): value(t), flag(true) {}
					void acquire(T t) {
						value = t;
						assert(!flag);
						flag = true;
					}
					void release() {
						assert(flag);
						flag = false;
					}
					bool check() const { return flag; }
				#else
					void acquire(T t) {
						value = t;
					}
					void release() {}
					bool check() const { return true; }
				#endif
			} item;

			Index	freeIdx;

			ItemU(T t): item(t) {}
		};

		using ItemVec = std::vector<ItemU>;
		ItemVec		_array;
		Index		_firstFree,		//!< 最初の空きブロックインデックス
					_nFree = 0;		//!< 空きブロック数

		public:
			IDPair() {
				static_assert(std::is_integral<T>::value, "typename T should be Integral type");
			}
			IDPair(const IDPair& p) = default;
			IDPair(IDPair&& p): _array(std::forward<ItemVec>(p._array)), _firstFree(p._firstFree), _nFree(p._nFree) {}
			Index take(T t) {
				if(_nFree == 0) {
					Index sz = _array.size();
					_array.emplace_back(t);
					return sz;
				}
				--_nFree;
				Index ret = _firstFree;
				_firstFree = _array[_firstFree].freeIdx;
				_array[ret].item.acquire(t);
				return ret;
			}
			T put(Index idx) {
				T ret = _array[idx].item.value;
				_array[idx].item.release();
				if(++_nFree != 1)
					_array[idx].freeIdx = _firstFree;
				_firstFree = idx;
				return ret;
			}
			void rewrite(Index idx, T newID) {
				assert(_array[idx].item.check());
				_array[idx].item.value = newID;
			}

			size_t size() const {
				return _array.size() - _nFree;
			}
			void clear() {
				_array.clear();
				_nFree = 0;
			}
	};
	//! 順序なし配列
	/*! 基本的にはstd::vectorそのまま
		要素を削除した時に順序を保証しない点だけが異なる */
	template <class T, class AL=std::allocator<T>>
	class noseq_vec : public std::vector<T, AL> {
		using vector = std::vector<T,AL>;
		public:
			using vector::vector;
			using vector::erase;
			void erase(typename vector::iterator itr) {
				if(vector::size() > 1 && itr != --vector::end()) {
					assert(itr < vector::end());
					// 削除対象が最後尾でなければ最後尾の要素と取り替える
					std::swap(*itr, vector::back());
				}
				assert(itr != vector::end());
				vector::pop_back();
			}
	};
	//! 順序なしのID付きリスト
	/*! 全走査を速く、要素の追加削除を速く(走査中はNG)、要素の順序はどうでもいい、あまり余計なメモリは食わないように・・というクラス */
	template <class T, class ID=unsigned int>
	class noseq_list {
		struct RPair {
			T	value;
			ID	uid;		//!< ユニークID。要素の削除をする際に使用

			RPair(RPair&& rp): value(std::forward<T>(rp.value)) {}
			RPair(T&& t, ID id): value(std::forward<T>(t)), uid(id) {}
			RPair(const T& t, ID id): value(t), uid(id) {}

			RPair& operator = (RPair&& r) {
				std::swap(value, r.value);
				std::swap(uid, r.uid);
				return *this;
			}
		};
		using NSArray = noseq_vec<RPair>;

		NSArray		_array;
		IDPair<ID>	_idList;

		public:
			using iterator = typename NSArray::iterator;
			using const_iterator = typename NSArray::const_iterator;
			using id_type = ID;
			noseq_list() = default;
			noseq_list(noseq_list&& sl): _array(std::forward<noseq_vec<T>>(sl._array)), _idList(std::forward<IDPair<ID>>(sl._idList)) {}

			template <class T2>
			ID add(T2&& t) {
				auto sz = _array.size();
				ID id = _idList.take(sz);
				_array.emplace_back(std::forward<T2>(t), id);
				return id;
			}
			void rem(ID id) {
				// idListから削除対象のnoseqインデックスを受け取る
				auto index = _idList.put(id);
				if(index != _array.size()-1) {
					auto& arB = _array.back();
					// UIDとindex対応の書き換え
					_idList.rewrite(arB.uid, index);
				}
				_array.erase(_array.begin()+index);
			}
			iterator begin() { return _array.begin(); }
			iterator end() { return _array.end(); }
			const_iterator cbegin() const { return _array.cbegin(); }
			const_iterator cend() const { return _array.cend(); }
			const_iterator begin() const { return _array.begin(); }
			const_iterator end() const { return _array.end(); }

			size_t size() const {
				return _array.size();
			}
			void clear() {
				_array.clear();
				_idList.clear();
			}
			bool empty() const {
				return _array.empty();
			}
	};
}
