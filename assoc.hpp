#pragma once
#include <algorithm>
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include "serialization/traits.hpp"
#include "sort.hpp"

namespace spn {
	//! ソート済みベクタ
	/*! 内部をソートされた状態に保つため要素の編集は基本的に不可
		もし要素を編集した場合は必ずre_sort()を呼ぶ */
	template <class T, class Pred=std::less<>, class CT=std::vector<T>>
	class AssocVec : public boost::serialization::traits<AssocVec<T,Pred,CT>,
								boost::serialization::object_serializable,
								boost::serialization::track_selectively>
	{
		private:
			using Vec = CT;
			Vec		_vec;

			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int /*ver*/) {
				ar & BOOST_SERIALIZATION_NVP(_vec);
			}

		protected:
			using VecItrC = typename Vec::const_iterator;
			VecItrC _cbegin() const { return _vec.cbegin(); }
			VecItrC _cend() const { return _vec.cend(); }

		public:
			using value_type = T;
			using cmp_type = Pred;

			AssocVec() = default;
			AssocVec(AssocVec&&) = default;
			AssocVec(const Vec& src): _vec(src) {
				std::sort(_vec.begin(), _vec.end());
			}
			AssocVec(Vec&& src): _vec(std::forward<Vec>(src)) {
				std::sort(_vec.begin(), _vec.end());
			}

			bool operator == (const AssocVec& av) const {
				return _vec == av._vec;
			}
			bool operator != (const AssocVec& av) const {
				return _vec != av._vec;
			}
			//! 要素を追加
			/*! \return 挿入されたソート後のインデックス */
			template <class T2>
			int insert(T2&& t) {
				T obj(std::forward<T2>(t));
				auto itrE = _vec.end();
				auto itr = _vec.begin();
				Pred pred;
				while(itr!=itrE && pred(*itr, obj))
					++itr;
				if(itr == itrE) {
					_vec.emplace_back(std::move(obj));
					itr = _vec.end()-1;
				} else
					itr = _vec.emplace(itr, std::move(obj));
				return itr - _vec.begin();
			}
			template <class... Ts>
			int emplace(Ts&&... ts) {
				return insert(value_type(std::forward<Ts>(ts)...));
			}
			//! 外部から要素の書き換えを行った際にソートし直す
			void re_sort() {
				// 前回と大体同じような並び順になっている事を想定し、単純挿入ソート
				insertion_sort(_vec.begin(), _vec.end(), Pred());
			}

			void pop_front() {
				_vec.erase(_vec.begin());
			}
			//! pop_frontと同じだが、消去される要素の値を返す
			T pop_frontR() {
				T ret(std::move(_vec.front()));
				_vec.erase(_vec.begin());
				return std::move(ret);
			}
			void pop_back() { _vec.pop_back(); }
			//! pop_backと同じだが、消去される要素の値を返す
			T pop_backR() {
				T ret(std::move(_vec.back()));
				_vec.pop_back();
				return std::move(ret);
			}
			void erase(int n) {
				_vec.erase(_vec.begin()+n);
			}
			void erase(typename Vec::iterator itr) {
				_vec.erase(itr);
			}
			void erase(typename Vec::const_iterator itr) {
				_vec.erase(itr);
			}
			const T& back() const { return _vec.back(); }
			const T& front() const { return _vec.front(); }

			size_t size() const { return _vec.size(); }
			void swap(AssocVec& v) noexcept {
				std::swap(_vec, v._vec);
			}
			const T& operator [](int n) const {
				return _vec[n];
			}
			typename Vec::const_iterator cbegin() const {
				return _vec.cbegin();
			}
			typename Vec::const_iterator cend() const {
				return _vec.cend();
			}
			bool empty() const {
				return _vec.empty();
			}
			void clear() {
				_vec.clear();
			}
	};
	//! std::pairのfirstをless比較
	template <class CMP>
	struct CmpFirst {
		template <class K, class T>
		bool operator()(const std::pair<K,T>& t0, const std::pair<K,T>& t1) const {
			return CMP()(t0.first, t1.first);
		}
	};
	//! キー付きソート済みベクタ
	/*! キーは固定のため、値の編集が可能 */
	template <class K, class T, class Pred=std::less<>, class CT=std::vector<std::pair<K,T>>>
	class AssocVecK : public AssocVec<std::pair<K,T>, CmpFirst<Pred>, CT> {
		public:
			using value_type = std::pair<K,T>;
			using Entry = value_type;
			using cmp_type = Pred;
			using ASV = AssocVec<value_type, CmpFirst<Pred>, CT>;

			T& back() {
				const Entry& ent = const_cast<ASV*>(this)->back();
				return const_cast<Entry&>(ent).second;
			}
			T& front() {
				const Entry& ent = const_cast<ASV*>(this)->front();
				return const_cast<Entry&>(ent).second;
			}
			T& operator [](int n) {
				const Entry& ent = (*const_cast<ASV*>(this))[n];
				return const_cast<Entry&>(ent).second;
			}
			const K& getKey(int n) const {
				return ASV::operator [](n).first;
			}

			typename ASV::VecItrC cbegin() const {
				return ASV::_cbegin();
			}
			typename ASV::VecItrC cend() const {
				return ASV::_cend();
			}
	};
}

