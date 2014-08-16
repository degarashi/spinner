#pragma once
#include <assert.h>

namespace spn {
	//! 汎用シングルトン
	template <typename T>
	class Singleton {
		private:
			static T* ms_singleton;
		public:
			Singleton() {
				assert(!ms_singleton || !"initializing error - already initialized");
				intptr_t offset = reinterpret_cast<intptr_t>(reinterpret_cast<T*>(1)) -
									reinterpret_cast<intptr_t>(static_cast<Singleton<T>*>(reinterpret_cast<T*>(1)));
				ms_singleton = reinterpret_cast<T*>(reinterpret_cast<intptr_t>(this) + offset);
			}
			virtual ~Singleton() {
				assert(ms_singleton || !"destructor error");
				ms_singleton = 0;
			}
			static T& _ref() {
				assert(ms_singleton || !"reference error");
				return *ms_singleton;
			}
	};
	template <typename T>
	T* Singleton<T>::ms_singleton = nullptr;
}

