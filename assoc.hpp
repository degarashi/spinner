#pragma once
#include <algorithm>
#include <vector>

namespace spn {
	//! ソート済みベクタ
	/*! 内部をソートされた状態に保つため要素の編集は不可 */
	template <class T, class Pred=std::less<T>, template<class,class> class CT=std::vector, class AL=std::allocator<T>>
	class AssocVec {
		using Vec = CT<T, AL>;
		Vec		_vec;

		public:
			AssocVec() = default;
			AssocVec(AssocVec&& v): _vec(std::forward<Vec>(v._vec)) {}
			AssocVec(const Vec& src): _vec(src) {
				std::sort(_vec.begin(), _vec.end());
			}
			AssocVec(Vec&& src): _vec(std::forward<Vec>(src)) {
				std::sort(_vec.begin(), _vec.end());
			}

			//! 要素を追加
			/*! \return 挿入されたソート後のインデックス */
			template <class T2>
			int insert(T2&& t) {
				auto itrE = _vec.end();
				auto itr = _vec.begin();
				Pred pred;
				while(itr!=itrE && pred(*itr, t))
					++itr;
				if(itr == itrE) {
					_vec.emplace_back(std::forward<T>(t));
					itr = _vec.end()-1;
				} else
					itr = _vec.emplace(itr, std::forward<T>(t));
				return itr - _vec.begin();
			}

			void pop_front() {
				_vec.erase(_vec.begin());
			}
			T pop_frontR() {
				T ret(std::move(_vec.front()));
				_vec.erase(_vec.begin());
				return std::move(ret);
			}
			void pop_back() { _vec.pop_back(); }
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
			const T& back() const { return _vec.back(); }
			const T& front() const { return _vec.front(); }

			size_t size() const { return _vec.size(); }
			void swap(AssocVec& v) noexcept {
				std::swap(_vec, v._vec);
			}
			const T& operator [](int n) const {
				return _vec[n];
			}
			typename Vec::const_iterator begin() const {
				return _vec.cbegin();
			}
			typename Vec::const_iterator end() const {
				return _vec.cend();
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
}
