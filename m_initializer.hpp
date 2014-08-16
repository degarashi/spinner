#pragma once
#include <utility>

namespace spn {
	//! 指定した順番で関数を呼び、クラスの初期化をする(マネージャの初期化用)
	/*! デストラクタは初期化の時と逆順で呼ばれる */
	template <class... Ts>
	struct MInitializer {
		MInitializer(bool) {}
		void release() {}
	};
	template <class T, class... Ts>
	struct MInitializer<T,Ts...> {
		bool		bTop;
		T			ptr;
		MInitializer<Ts...>	other;

		template <class TA, class... TsA>
		MInitializer(bool btop, TA&& t, TsA&&... tsa): bTop(btop), ptr(t()), other(false, std::forward<TsA>(tsa)...) {}
		~MInitializer() {
			if(bTop)
				release();
		}
		void release() {
			if(ptr) {
				other.release();
				delete ptr;
				ptr = nullptr;
			}
		}
	};
	//! MInitializerのヘルパ関数: newしたポインタを返すラムダ式を渡す
	/*! MInitializerF([](){ return Manager0(); },<br>
					[](){ return Another(); });<br>
		のように呼ぶ */
	template <class... Ts>
	auto MInitializerF(Ts&&... ts) -> MInitializer<decltype(std::declval<Ts>()())...> {
		return MInitializer<decltype(std::declval<Ts>()())...>(true, std::forward<Ts>(ts)...);
	}
}

