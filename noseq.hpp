#pragma once
#include <vector>
#include <type_traits>
#include <algorithm>
#define BOOST_PP_VARIADICS 1
#include <boost/variant.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/traits.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "serialization/traits.hpp"
#include "type.hpp"
#include "optional.hpp"

namespace boost {
	namespace serialization {
		template <class Archive>
		void serialize(Archive& ar, boost::blank& blk, const unsigned int) {
			// 何も出力しない
		}
	}
}
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
	struct TypeWrap : boost::serialization::traits<TypeWrap<T,N>, boost::serialization::object_serializable, boost::serialization::track_never, 0> {
		T value;

		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar & value;
		}

		TypeWrap() = default;
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
	template <class T, template <class> class Allocator=std::allocator, class IDType=unsigned int, unsigned int MaxID=std::numeric_limits<IDType>::max()>
	class noseq_list {
		using ThisType = noseq_list<T,Allocator,IDType,MaxID>;
		using ID = typename std::make_unsigned<IDType>::type;
		constexpr static bool Validation(ID id) {
			return id < MaxID;
		}
		using RT = typename std::remove_reference<T>::type;
		using OPValue = Optional<T>;
		//! ユーザーの要素を格納
		struct UData {
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
			bool operator == (const UData& ud) const {
				return value == ud.value &&
						uid == ud.uid;
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
			//! deserialize用
			Entry(OPValue&& value, ID uid, IDS ids): udata(std::move(value), uid), ids(ids) {}
			operator typename TheType<T>::type () { return *udata.value; }
			operator typename TheType<T>::ctype () const { return *udata.value; }

			bool operator == (const Entry& e) const {
				return udata == e.udata &&
						ids == e.ids;
			}
		};
		using Array = std::vector<Entry, Allocator<Entry>>;
		Array		_array;
		ID			_nFree = 0,		//!< 空きブロック数
					_firstFree;		//!< 最初の空きブロックインデックス
		//! 現在削除処理中かのフラグ
		bool		_bRemoving = false;
		using RemList = std::vector<ID>;
		RemList		_remList;

		friend class boost::serialization::access;
		BOOST_SERIALIZATION_SPLIT_MEMBER();
		template <class Archive>
		void load(Archive& ar, const unsigned int ver) {
			AssertP(Trap, !_bRemoving && _remList.empty())
			bool bIDS;
			size_t nEnt;
			ar & bIDS & nEnt & _nFree & _firstFree;

			ID uid;
			IDS ids;
			if(bIDS) {
				// IDだけを上書き
				if(_array.size() > nEnt)
					_array.erase(_array.begin()+nEnt, _array.end());
				else {
					// スペース確保
					while(_array.size() < nEnt)
						_array.emplace_back(OPValue(), 0, boost::blank());
				}
				for(size_t i=0 ; i<nEnt ; i++) {
					ar & uid & ids;

					auto& ent = _array[i];
					ent.udata.uid = uid;
					ent.ids = ids;
				}
			} else {
				_array.clear();
				_array.reserve(nEnt);
				OPValue value;
				for(size_t i=0 ; i<nEnt ; i++) {
					// Entryの復元
					ar & value & uid & ids;
					_array.emplace_back(std::move(value), uid, ids);
				}
			}
		}
		template <class Archive>
		void save(Archive& ar, const unsigned int ver) const {
			AssertP(Trap, !_bRemoving && _remList.empty())
			size_t nEnt = _array.size();
			ar & s_bIDS & nEnt & _nFree & _firstFree;
			if(s_bIDS) {
				// IDのみの出力
				for(auto& p : _array)
					ar & p.udata.uid & p.ids;
			} else {
				// 通常の出力
				for(auto& p : _array)
					ar & p.udata.value & p.udata.uid & p.ids;
			}
		}
		//! IDだけ出力するフラグ
		static bool s_bIDS;
		struct IDSType : boost::serialization::traits<IDSType,
			boost::serialization::object_serializable,
			boost::serialization::track_never>
		{
			const ThisType* _ths;
			BOOST_SERIALIZATION_SPLIT_MEMBER();
			// セーブ関数しか用意しない
			template <class Archive>
			void save(Archive& ar, const unsigned int) const {
				bool tmp = s_bIDS;
				s_bIDS = true;
				ar & (*_ths);
				s_bIDS = tmp;
			}
		};
		static IDSType s_ids;

		public:
			template <class Archive>
			void _load_partial(Archive& ar, const std::vector<bool>& chk) {
				size_t nEnt = chk.size();
				// スペース確保
				while(_array.size() < nEnt)
					_array.emplace_back(OPValue(), 0, boost::blank());

				size_t len;
				std::string buff;
				std::istringstream ss;
				for(size_t i=0 ; i<nEnt ; i++) {
					ar & len;
					buff.resize(len);
					ar.load_binary(&buff[0], len);

					if(!chk[i]) {
						ss.str(buff);
						OPValue value;
						boost::archive::binary_iarchive ia(ss, boost::archive::no_header);
						ia >> value;
						_array[i].udata.value = std::move(value);
					}
				}
			}
			const IDSType& asIDSType() const {
				s_ids._ths = this;
				return s_ids; }
			using entry_type = Entry;
			using iterator = AdaptItrBase<T, typename Array::iterator>;
			using const_iterator = AdaptItrBase<const T, typename Array::const_iterator>;
			using reverse_iterator = AdaptItrBase<T, std::reverse_iterator<typename Array::iterator>>;
			using const_reverse_iterator = AdaptItrBase<const T, std::reverse_iterator<typename Array::const_iterator>>;

			using id_type = ID;
			noseq_list() {
				static_assert(std::is_integral<IDType>::value, "typename ID should be Integral type");
			}
			noseq_list(noseq_list&& sl): _array(std::move(sl._array)), _nFree(sl._nFree), _firstFree(sl._firstFree) {
				AssertP(Trap, !_bRemoving && _remList.empty())
			}

			template <class... Ts>
			ID emplace(Ts&&... ts) {
				return add(T(std::forward<Ts>(ts)...));
			}
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
				ID id = add(OPValue::AsInitialized);
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
			//! 主にライブラリ内部用途
			const OPValue& getValueOP(int index) const {
				AssertP(Trap, _array.size() > index)
				return _array[index].udata.value;
			}
			//! IDが有効か判定
			bool has(ID uindex) const {
				if(!Validation(uindex) || _array.size() <= uindex || _array[uindex].ids.which() != 1)
					return false;
				return true;
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
			//! 主にデバッグ用。内部状態も含めて比較
			bool operator == (const noseq_list& lst) const {
				AssertP(Trap, !_bRemoving)
				return _nFree == lst._nFree &&
						_firstFree == lst._firstFree &&
						_remList == lst._remList &&
						_array == lst._array;
			}
	};
	template <class T, template <class> class Allocator, class IDType, unsigned int MaxID>
	bool noseq_list<T,Allocator,IDType,MaxID>::s_bIDS = false;
	template <class T, template <class> class Allocator, class IDType, unsigned int MaxID>
	typename noseq_list<T,Allocator,IDType,MaxID>::IDSType noseq_list<T,Allocator,IDType,MaxID>::s_ids{};
}
BOOST_CLASS_VERSION_TEMPLATE((class)(template <class> class)(class)(unsigned int), spn::noseq_list, 0)
