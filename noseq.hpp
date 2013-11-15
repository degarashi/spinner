#pragma once
#include <vector>
#include <type_traits>
#include <algorithm>
#define BOOST_PP_VARIADICS 1
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include "type.hpp"
#include "optional.hpp"

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
	// TODO: 間に合わせの名前なので後でちゃんとした名前にする
	template <class T>
	struct TheType {
		using T2 = typename std::remove_reference<T>::type;
		using ctype = const typename std::remove_const<T2>::type&;
		using type = T2&;
		using pointer = T2*;
	};
	template <class T>
	struct TheType<T*> {
		using ctype = const typename std::remove_const<T>::type*;
		using type = T*&;
		using pointer = T**;
	};
	template <class DT, class REV>
	class AdaptItrBase : public REV {
		using Itr = REV;
		public:
			using Itr::Itr;
			using value_type = DT;
			using pointer = typename TheType<DT>::pointer;
			using reference = typename TheType<DT>::type;

			AdaptItrBase() = default;
			AdaptItrBase(const Itr& itr): Itr(itr) {}
			reference operator *() const {
				return (reference)Itr::operator*();
			}
	};

	//! 順序なしのID付きリスト
	/*! 全走査を速く、要素の追加削除を速く(走査中はNG)、要素の順序はどうでもいい、あまり余計なメモリは食わないように・・というクラス */
	template <class T, class IDType=unsigned int, unsigned int MaxID=std::numeric_limits<IDType>::max()>
	class noseq_list {
		using ID = typename std::make_unsigned<IDType>::type;
		constexpr static bool Validation(ID id) {
			return id < MaxID;
		}
		using RT = typename std::remove_reference<T>::type;
		//! ユーザーの要素を格納
		struct UData {
			using OPValue = Optional<T>;
			OPValue		value;
			ID			uid;		//!< ユニークID。要素の配列内移動をする際に使用

			UData(UData&& rp): value(std::forward<OPValue>(rp.value)), uid(rp.uid) {}
			template <class T2>
			UData(T2&& t, ID id): value(std::forward<T2>(t)), uid(id) {}

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

			template <class T2>
			Entry(T2&& t, ID idx): udata(std::forward<T2>(t),idx), ids(ObjID(idx)) {}
			operator typename TheType<T>::type () { return *udata.value; }
			operator typename TheType<T>::ctype () const { return *udata.value; }
		};
		using Array = std::vector<Entry>;
		Array		_array;
		ID			_nFree = 0,		//!< 空きブロック数
					_firstFree;		//!< 最初の空きブロックインデックス
		//! 現在削除処理中かのフラグ
		bool		_bRemoving = false;
		using RemList = std::vector<ID>;
		RemList		_remList;

		public:
			using iterator = AdaptItrBase<T, typename Array::iterator>;
			using const_iterator = AdaptItrBase<const T, typename Array::const_iterator>;
			using reverse_iterator = AdaptItrBase<T, std::reverse_iterator<typename Array::iterator>>;
			using const_reverse_iterator = AdaptItrBase<const T, std::reverse_iterator<typename Array::const_iterator>>;

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
				AssertP(Trap, Validation(uindex), "invalid resource number %1%", uindex)
				if(!_bRemoving) {
					_bRemoving = true;

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
					_array[backI].udata.value = none;
					// フリーリストをつなぎ替える
					ids = FreeID(_firstFree);
					_firstFree = uindex;
					++_nFree;

					_bRemoving = false;
					while(!_remList.empty()) {
						ID tmp = _remList[0];
						_remList.erase(_remList.begin());
						rem(tmp);
					}
				} else
					_remList.push_back(uindex);
			}
			using Ref = decltype(*_array[0].udata.value);
			//! 要素の領域を先に取得
			std::pair<ID,Ref> alloc() {
				ID id = add(UData::OPValue::AsInitialized);
				return std::pair<ID,Ref>(id, get(id));
			}
			Ref get(ID uindex) {
				AssertP(Trap, Validation(uindex), "invalid resource number %1%", uindex)
				ID idx = boost::get<ObjID>(_array[uindex].ids);
				return *_array[idx].udata.value;
			}
			const Ref get(ID uindex) const {
				AssertP(Trap, Validation(uindex), "invalid resource number %1%", uindex)
				return const_cast<noseq_list*>(this)->get(uindex);
			}

			// イテレータの範囲はフリー要素を勘定に入れる
			iterator begin() { return _array.begin(); }
			iterator end() { return _array.end()-_nFree; }
			reverse_iterator rbegin() { return _array.rbegin()+_nFree; }
			reverse_iterator rend() { return _array.rend(); }
			const_iterator cbegin() const { return _array.cbegin(); }
			const_iterator cend() const { return _array.cend()-_nFree; }
			const_iterator begin() const { return _array.begin(); }
			const_iterator end() const { return _array.end()-_nFree; }
			const_reverse_iterator crbegin() const { return const_cast<noseq_list*>(this)->rbegin(); }
			const_reverse_iterator crend() const { return const_cast<noseq_list*>(this)->rend(); }

			//! 主にResMgrから開放時の処理をする為に使用
			template <class CB>
			void iterate(CB cb) {
				int n = _array.size();
				for(int i=0 ; i<n ; i++) {
					auto& ent = _array[i];
					// エントリが有効な場合のみコールバックを呼ぶ
					if(ent.ids.which() == 1) {
						ID id = boost::get<ObjID>(ent.ids);
						cb(_array[id].udata.value);
					}
				}
			}

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
