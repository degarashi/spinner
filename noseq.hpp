#pragma once
#include <vector>
#include <type_traits>
#include <algorithm>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include "type.hpp"
namespace spn {
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
	//! noseq_listでboost::optionalを使おうとしたらよくわからないエラーが出たので自作してしまった
	template <class T>
	class Optional {
		private:
			uint8_t	_buffer[sizeof(T)];
			bool	_bInit;

			void _release() {
				if(_bInit) {
					reinterpret_cast<T*>(_buffer)->~T();
					_bInit = false;
				}
			}
			T* _castT() { return reinterpret_cast<T*>(_buffer); }
			const T* _castT() const { return reinterpret_cast<const T*>(_buffer); }

		public:
			Optional(): _bInit(false) {}
			Optional(boost::none_t): _bInit(false) {}
			template <class T2>
			Optional(T2&& t): _bInit(true) {
				new(_buffer) T(std::forward<T2>(t)); }
			Optional(Optional<T>&& t): _bInit(t) {
				if(t) {
					new(_buffer) T(std::move(*t._castT()));
					t._bInit = false;
				}
			}
			Optional(const Optional<T>& t): _bInit(t) {
				if(t)
					new(_buffer) T(*t._castT());
			}
			~Optional() {
				_release();
			}

			T& get() {
				if(!*this)
					throw boost::bad_get();
				return *_castT();
			}
			const T& get() const {
				return const_cast<Optional<T>*>(this)->get();
			}
			T& operator * () {
				return get();
			}
			const T& operator * () const {
				return get();
			}
			operator bool () const {
				return _bInit;
			}

			template <class T2>
			Optional& operator = (T2&& t) {
				_release();
				new(_buffer) T(std::forward<T2>(t));
				_bInit = true;
				return *this;
			}
			Optional& operator = (boost::none_t) noexcept {
				_release();
				return *this;
			}
			Optional& operator = (const Optional<T>& t) {
				_release();
				if(t) {
					new(_buffer) T(t.value);
					_bInit = true;
				}
				return *this;
			}
			Optional& operator = (Optional<T>&& t) noexcept {
				_release();
				if(t) {
					new(_buffer) T(std::move(*t._castT()));
					t._bInit = false;
					_bInit = true;
				}
				return *this;
			}
	};
	//! 同じ型を(Nを変えて)違う型として扱う為のラップクラス
	template <class T, int N>
	struct TypeWrap {
		T value;

		explicit TypeWrap(const T& id): value(id) {}
		explicit TypeWrap(T&& id): value(std::forward<T>(id)) {}
		operator T& () { return value; }
		operator const T& () const { return value; }
		TypeWrap& operator = (const T& id) {
			value = id;
			return *this;
		}
		TypeWrap& operator = (T&& id) {
			value = std::forward<T>(id);
			return *this;
		}
	};
	//! 順序なしのID付きリスト
	/*! 全走査を速く、要素の追加削除を速く(走査中はNG)、要素の順序はどうでもいい、あまり余計なメモリは食わないように・・というクラス */
	template <class T, class IDType=unsigned int>
	class noseq_list {
		using ID = typename std::make_unsigned<IDType>::type;
		//! ユーザーの要素を格納
		struct UData {
			using OPValue = Optional<T>;
			OPValue		value;
			ID			uid;		//!< ユニークID。要素の配列内移動をする際に使用

			UData(UData&& rp): value(std::forward<OPValue>(rp.value)), uid(rp.uid) {}
			UData(T&& t, ID id): value(std::forward<T>(t)), uid(id) {}
			UData(const T& t, ID id): value(t), uid(id) {}

			UData& operator = (UData&& r) {
				std::swap(value, r.value);
				std::swap(uid, r.uid);
				return *this;
			}
		};
		struct FreeID : TypeWrap<ID,0> {using TypeWrap<ID,0>::TypeWrap;};
		struct ObjID : TypeWrap<ID,1> {using TypeWrap<ID,1>::TypeWrap;};
		/*! ObjID = UserDataの格納先インデックス
			FreeID = 次の空きインデックス */
		using IDS = boost::variant<boost::blank, ObjID, FreeID>;

		struct Entry {
			UData	udata;
			IDS		ids;

			Entry(T&& t, ID idx): udata(std::forward<T>(t),idx), ids(ObjID(idx)) {}
		};
		using Array = std::vector<Entry>;
		Array		_array;
		ID			_nFree = 0,		//!< 空きブロック数
					_firstFree;		//!< 最初の空きブロックインデックス

		public:
			class iterator : public Array::iterator {
				using Itr = typename Array::iterator;
				public:
					using Itr::Itr;
					T& operator * () {
						auto& ent = *(Itr&)(*this);
						return *ent.udata.value;
					}
			};
			class const_iterator : public Array::const_iterator {
				using Itr = typename Array::const_iterator;
				public:
					const_iterator() = default;
					const_iterator(const typename Array::const_iterator& itr): Array::const_iterator(itr) {}
					const T& operator * () const {
						auto& ent = *(Itr&)(*this);
						return *ent.udata.value;
					}
			};

			using id_type = ID;
			noseq_list() {
				static_assert(std::is_integral<IDType>::value, "typename ID should be Integral type");
			}
			noseq_list(noseq_list&& sl): _array(std::forward<noseq_vec<T>>(sl._array)), _nFree(sl._nFree), _firstFree(sl._firstFree) {}

			template <class T2>
			ID add(T2&& t) {
				if(_nFree == 0) {
					// 空きが無いので配列の拡張
					ID sz = _array.size();
					_array.emplace_back(std::forward<T2>(t), sz);
					return sz;
				}
				ID ret = _firstFree;					// IDPairを書き込む場所
				ID objI = _array.size() - _nFree;		// ユーザーデータを書き込む場所
				--_nFree;
				auto& objE = _array[objI].udata;
				objE.value = std::forward<T2>(t);		// ユーザーデータの書き込み
				objE.uid = ret;

				auto& idE = _array[ret].ids;
				_firstFree = boost::get<FreeID>(idE);	// フリーリストの先頭を書き換え
				idE = ObjID(objI);						// IDPairの初期化
				return ret;
			}
			void rem(ID uindex) {
				auto& ids = _array[uindex].ids;
				auto& objI = boost::get<ObjID>(ids);	// 削除対象のnoseqインデックスを受け取る
				ID backI = _array.size()-_nFree-1;
				if(objI != backI) {
					// 最後尾と削除予定の要素を交換
					std::swap(_array[backI].udata, _array[objI].udata);
					// UIDとindex対応の書き換え
					_array[_array[objI].udata.uid].ids = ObjID(objI);
				}
				// 要素を解放
				_array[backI].udata.value = boost::none;
				// フリーリストをつなぎ替える
				ids = FreeID(_firstFree);
				_firstFree = uindex;
				++_nFree;
			}
			T& get(ID uindex) {
				ID idx = boost::get<ObjID>(_array[uindex].ids);
				return *_array[idx].udata.value;
			}
			const T& get(ID uindex) const {
				return const_cast<noseq_list*>(this)->get(uindex);
			}

			iterator begin() { return _array.begin(); }
			iterator end() { return _array.end()-_nFree; }
			const_iterator cbegin() const { return _array.cbegin(); }
			const_iterator cend() const { return _array.cend()-_nFree; }
			const_iterator begin() const { return _array.begin(); }
			const_iterator end() const { return _array.end()-_nFree; }

			size_t size() const {
				return _array.size() - _nFree;
			}
			void clear() {
				_array.clear();
				_nFree = 0;
			}
			bool empty() const {
				return size() == 0;
			}
	};
}
