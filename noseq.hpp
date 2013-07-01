#pragma once
#include <vector>
#include <type_traits>
#include <algorithm>
#define DEBUG
namespace spn {
	template <class T>
	class IDPair {
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
		};

		using ItemVec = std::vector<Item>;
		using IDVec = std::vector<int>;
		ItemVec		_array;
		IDVec		_idStack;

		public:
			IDPair() {
				static_assert(std::is_integral<T>::value, "typename T should be Integral type");
			}
			IDPair(const IDPair& p) = default;
			IDPair(IDPair&& p): _array(std::forward<ItemVec>(p._array)), _idStack(std::forward<IDVec>(p._idStack)) {}
			int take(T t) {
				if(_idStack.empty()) {
					int sz = _array.size();
					_array.emplace_back(t);
					return sz;
				}
				int ret = _idStack.back();
				_idStack.pop_back();
				_array[ret].acquire(t);
				return ret;
			}
			T put(int idx) {
				T ret = _array[idx].value;
				_array[idx].release();
				_idStack.push_back(idx);
				return ret;
			}
			void rewrite(int idx, T newID) {
				assert(_array[idx].check());
				_array[idx].value = newID;
			}

			size_t size() const {
				return _array.size() - _idStack.size();
			}
			void clear() {
				_array.clear();
				_idStack.clear();
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
		using NSItr = typename NSArray::iterator;
		using NSItrC = typename NSArray::const_iterator;

		NSArray		_array;
		IDPair<ID>	_idList;

		public:
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
					auto& ar = _array[index];
					auto& arB = _array.back();
					// UIDとindex対応の書き換え
					_idList.rewrite(arB.uid, index);
				}
				_array.erase(_array.begin()+index);
			}
			NSItr begin() { return _array.begin(); }
			NSItr end() { return _array.end(); }
			NSItrC cbegin() const { return _array.cbegin(); }
			NSItrC cend() const { return _array.cend(); }
			NSItrC begin() const { return _array.begin(); }
			NSItrC end() const { return _array.end(); }

			size_t size() const {
				return _array.size();
			}
			void clear() {
				_array.clear();
				_idList.clear();
			}
	};
}
