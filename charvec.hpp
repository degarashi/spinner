#pragma once
#include <vector>

namespace spn {
	template <class T>
	class CharRange {
		private:
			T		_c0, _c1;
		public:
			CharRange(const T& c0, const T& c1):
				_c0(std::min(c0,c1)), _c1(std::max(c0,c1))
			{}
			CharRange(const T& c0): _c0(c0), _c1(c0) {}
			int size() const {
				return _c1 - _c0 + 1;
			}
			T* output(T* dst) const {
				auto cur = _c0,
					curE = _c1;
				for(;;) {
					*dst++ = static_cast<T>(cur);
					if(cur++ == curE)
						break;
				}
				return dst;
			}
	};
	template <class T>
	struct CharVec {
		using CharV = std::vector<T>;
		CharV	_charV;

		CharVec(std::initializer_list<CharRange<T>> il) {
			int count = 0;
			for(auto& cr : il)
				count += cr.size();

			_charV.resize(count);
			auto* ptr = _charV.data();
			for(auto& cr : il)
				ptr = cr.output(ptr);
		}
		int size() const {
			return _charV.size();
		}
		const T& get(int n) const {
			return _charV[n];
		}
	};
}

