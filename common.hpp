#pragma once
#include <tuple>
#include <type_traits>
#include <cmath>
#include <limits>

namespace spn {
	//! コンパイル時数値計算 & 比較
	template <int M, int N>
	struct TValue {
		enum { add = M+N,
			sub = M-N,
			less = (M>N) ? N : M,
			great = (M>N) ? M : N,
			less_eq = (M<=N) ? 1 : 0,
			great_eq = (M>=N) ? 1 : 0,
			lesser = (M<N) ? 1 : 0,
			greater = (M>N) ? 1 : 0,
			equal = M==N ? 1 : 0
		};
	};
	//! bool -> std::true_type or std::false_type
	template <int V>
	using TFCond = typename std::conditional<V!=0, std::true_type, std::false_type>::type;
	//! 2つの定数の演算結果をstd::true_type か std::false_typeで返す
	template <int N, int M>
	struct NType {
		using t_and = TFCond<(N&M)>;
		using t_or = TFCond<(N|M)>;
		using t_xor = TFCond<(N^M)>;
		using t_nand = TFCond<((N&M) ^ 0x01)>;
		using less = TFCond<(N<M)>;
		using great = TFCond<(N>M)>;
		using equal = TFCond<N==M>;
		using not_equal = TFCond<N!=M>;
		using less_eq = TFCond<(N<=M)>;
		using great_eq = TFCond<(N>=M)>;
	};
	//! 2つのintegral_constant<bool>を論理演算
	template <class T0, class T1>
	struct TType {
		constexpr static int I0 = std::is_same<T0, std::true_type>::value,
							I1 = std::is_same<T1, std::true_type>::value;
		using t_and = TFCond<I0&I1>;
		using t_or = TFCond<I0|I1>;
		using t_xor = TFCond<I0^I1>;
		using t_nand = TFCond<(I0&I1) ^ 0x01>;
	};
	//! 条件式の評価結果がfalseの場合、マクロ定義した箇所には到達しないことを保証
	#define __assume(cond) do { if (!(cond)) __builtin_unreachable(); } while (0);
	//! 最大値を取得
	template <int... N>
	struct TMax {
		constexpr static int result = 0; };
	template <int N0, int... N>
	struct TMax<N0, N...> {
		constexpr static int result = N0; };
	template <int N0, int N1, int... N>
	struct TMax<N0, N1, N...> {
		constexpr static int result = TValue<N0, TMax<N1, N...>::result>::great; };
	//! SFINAEで関数を無効化する際に使うダミー変数
	static void* Enabler;
	//! コンパイル時定数で数値のN*10乗を計算
	template <class T, int N, typename std::enable_if<N==0>::type*& = Enabler>
	constexpr T ConstantPow10(T value=1, std::integral_constant<int,N>* =nullptr) {
		return value;
	}
	template <class T, int N, typename std::enable_if<(N<0)>::type*& = Enabler>
	constexpr T ConstantPow10(T value=1, std::integral_constant<int,N>* =nullptr) {
		return ConstantPow10(value/10, (std::integral_constant<int,N+1>*)nullptr);
	}
	template <class T, int N, typename std::enable_if<(N>0)>::type*& = Enabler>
	constexpr T ConstantPow10(T value=1.f, std::integral_constant<int,N>* =nullptr) {
		return ConstantPow10(value*10, (std::integral_constant<int,N-1>*)nullptr);
	}

	//! std::tupleのハッシュを計算
	struct TupleHash {
		template <class Tup>
		static size_t get(size_t value, const Tup& /*tup*/, std::integral_constant<int,-1>) { return value; }
		template <class Tup, int N>
		static size_t get(size_t value, const Tup& tup, std::integral_constant<int,N>) {
			const auto& t = std::get<N>(tup);
			size_t h = std::hash<typename std::decay<decltype(t)>::type>()(t);
			value = (value ^ (h<<(h&0x07))) ^ (h>>3);
			return get(value, tup, std::integral_constant<int,N-1>());
		}
		template <class... Ts>
		size_t operator()(const std::tuple<Ts...>& tup) const {
			return get(0xdeadbeef * 0xdeadbeef, tup, std::integral_constant<int,sizeof...(Ts)-1>());
		}
	};

	//! クラスがwidthフィールドを持っていればintegral_constantでそれを返す
	template <class T>
	std::integral_constant<int,T::width> _GetWidthT(decltype(T::width)*);
	template <class T>
	std::integral_constant<int,0> _GetWidthT(...);
	template <class T>
	decltype(_GetWidthT<T>(nullptr)) GetWidthT();
	//! クラスがheightフィールドを持っていればintegral_constantでそれを返す
	template <class T>
	std::integral_constant<int,T::height> _GetHeightT(decltype(T::height)*);
	template <class T>
	std::integral_constant<int,0> _GetHeightT(...);
	template <class T>
	decltype(_GetHeightT<T>(nullptr)) GetHeightT();
	//! クラスがwidthとheightフィールドを持っていればintegral_pairでそれを返す
	template <class T, T val0, T val1>
	using integral_pair = std::pair<std::integral_constant<T, val0>,
									std::integral_constant<T, val1>>;
	template <class T>
	integral_pair<int,T::height, T::width> _GetWidthHeightT(decltype(T::width)*, decltype(T::height)*);
	template <class T>
	integral_pair<int,0, T::width> _GetWidthHeightT(decltype(T::width)*, ...);
	template <class T>
	integral_pair<int,0,0> _GetWidthHeightT(...);
	template <class T>
	decltype(_GetWidthHeightT<T>(nullptr,nullptr)) GetWidthHeightT();

	// 値比較(widthメンバ有り)
	template <class T, class CMP, int N>
	bool _EqFunc(const T& v0, const T& v1, CMP&& cmp, integral_pair<int,0,N>) {
		for(int i=0 ; i<N ; i++) {
			if(!cmp(v0.m[i], v1.m[i]))
				return false;
		}
		return true;
	}
	// 値比較(width & heightメンバ有り)
	template <class T, class CMP, int M, int N>
	bool _EqFunc(const T& v0, const T& v1, CMP&& cmp, integral_pair<int,M,N>) {
		for(int i=0 ; i<M ; i++) {
			for(int j=0 ; j<N ; j++) {
				if(!cmp(v0.ma[i][j], v1.ma[i][j]))
					return false;
			}
		}
		return true;
	}
	// 値比較(単一値)
	template <class T, class CMP>
	bool _EqFunc(const T& v0, const T& v1, CMP&& cmp, integral_pair<int,0,0>) {
		return cmp(v0, v1);
	}

	//! 絶対値の誤差による等値判定
	/*! \param[in] val value to check
		\param[in] vExcept target value
		\param[in] vEps value threshold */
	template <class T, class T2>
	bool EqAbs(const T& val, const T& vExcept, T2 vEps = std::numeric_limits<T>::epsilon()) {
		auto fnCmp = [vEps](const auto& val, const auto& except){ return std::fabs(except-val) <= vEps; };
		return _EqFunc(val, vExcept, fnCmp, decltype(GetWidthHeightT<T>())());
	}
	template <class T, class... Ts>
	bool EqAbsT(const std::tuple<Ts...>& /*tup0*/, const std::tuple<Ts...>& /*tup1*/, const T& /*epsilon*/, std::integral_constant<int,-1>*) {
		return true;
	}
	//! std::tuple全部の要素に対してEqAbsを呼ぶ
	template <class T, int N, class... Ts, typename std::enable_if<(N>=0)>::type*& = Enabler>
	bool EqAbsT(const std::tuple<Ts...>& tup0, const std::tuple<Ts...>& tup1, const T& epsilon, std::integral_constant<int,N>*) {
		return EqAbs(std::get<N>(tup0), std::get<N>(tup1), epsilon)
				&& EqAbsT(tup0, tup1, epsilon, (std::integral_constant<int,N-1>*)nullptr);
	}
	template <class... Ts, class T>
	bool EqAbs(const std::tuple<Ts...>& tup0, const std::tuple<Ts...>& tup1, const T& epsilon) {
		return EqAbsT<T>(tup0, tup1, epsilon, (std::integral_constant<int,sizeof...(Ts)-1>*)nullptr);
	}

	//! 浮動少数点数の値がNaNになっているか
	template <class T, class = typename std::enable_if<std::is_floating_point<T>::value>::type>
	bool IsNaN(const T& val) {
		return !(val>=T(0)) && !(val<T(0)); }
	//! 浮動少数点数の値がNaN又は無限大になっているか
	template <class T, class = typename std::enable_if<std::is_floating_point<T>::value>::type>
	bool IsOutstanding(const T& val) {
		auto valA = std::fabs(val);
		return valA==std::numeric_limits<float>::infinity() || IsNaN(valA); }

	//! 値飽和
	template <class T>
	T Saturate(const T& val, const T& minV, const T& maxV) {
		if(val > maxV)
			return maxV;
		if(val < minV)
			return minV;
		return val;
	}
	template <class T>
	T Saturate(const T& val, const T& range) {
		return Saturate(val, -range, range);
	}
	//! 値の範囲判定
	template <class T>
	bool IsInRange(const T& val, const T& vMin, const T& vMax) {
		return val>=vMin && val<=vMax;
	}
	template <class T>
	bool IsInRange(const T& val, const T& vMin, const T& vMax, const T& vEps) {
		return IsInRange(val, vMin-vEps, vMax+vEps);
	}

	//! std::tupleの要素ごとの距離(EqAbs)比較
	template <class T, int NPow>
	struct TupleNear {
		template <class P>
		bool operator()(const P& t0, const P& t1) const {
			return EqAbs(t0, t1, spn::ConstantPow10<T,NPow>());
		}
	};

	template <size_t N>
	struct GetCountOf_helper {
		using type = char [N];
	};
	template <class T, size_t N>
	typename GetCountOf_helper<N>::type& GetCountOf(T (&)[N]);
	//! 配列の要素数を取得する (配列でない場合はエラー)
	#define countof(e)		sizeof(::spn::GetCountOf(e))

	template <class T>
	char GetCountOfNA(T);
	template <class T, size_t N>
	typename GetCountOf_helper<N>::type& GetCountOfNA(T (&)[N]);
	//! 配列の要素数を取得する (配列でない場合は1が返る)
	#define countof_na(e)	sizeof(::spn::GetCountOfNA(e))
}

