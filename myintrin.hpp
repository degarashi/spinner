#pragma once
#include <intrin.h>
#if defined(SSE_LEVEL) && SSE_LEVEL > 0
	#if SSE_LEVEL >= 3
		#include <pmmintrin.h>
	#elif SSE_LEVEL == 2
		#include <emmintrin.h>
	#else
		#include <xmmintrin.h>
	#endif

	#define reg64i			__m64
	#define reg128			__m128
	#define reg128i			__m128i
	#define reg128d			__m128d
	#define reg_load_ps		_mm_load_ps
	#define reg_loadu_ps	_mm_loadu_ps
	#define reg_load1_ps	_mm_load1_ps
	#define reg_load_ss		_mm_load_ss
	#define reg_load_ps1	_mm_load_ps1
	#define reg_loadl_pi	_mm_loadl_pi
	#define reg_set_ps		_mm_set_ps
	#define reg_set1_ps		_mm_set1_ps
	#define reg_setzero_ps	_mm_setzero_ps
	#define reg_setr_ps		_mm_setr_ps

	#define reg_store_ss	_mm_store_ss
	#define reg_store_ps	_mm_store_ps
	#define reg_storeu_ps	_mm_storeu_ps
	#define reg_storel_pi	_mm_storel_pi

	#define reg_add_ps		_mm_add_ps
	#define reg_sub_ps		_mm_sub_ps
	#define reg_mul_ps		_mm_mul_ps
	#define reg_div_ps		_mm_div_ps
	#define reg_and_ps		_mm_and_ps
	#define reg_or_ps		_mm_or_ps
	#define reg_xor_ps		_mm_xor_ps
	#define reg_andnot_ps	_mm_andnot_ps

	#define reg_move_ss		_mm_move_ss
	#define reg_movemask_ps	_mm_movemask_ps
	#define reg_max_ps		_mm_max_ps
	#define reg_min_ps		_mm_min_ps
	#define reg_unpacklo_ps	_mm_unpacklo_ps
	#define reg_unpackhi_ps	_mm_unpackhi_ps
	#define reg_packs_epi32		_mm_packs_epi32
	#define reg_packus_epi16	_mm_packus_epi16
	#define reg_shuffle_ps	_mm_shuffle_ps
	#define _REG_SHUFFLE	_MM_SHUFFLE

	#define reg_rcp_ps		_mm_rcp_ps
	#define reg_sqrt_ps		_mm_sqrt_ps
	#define reg_sqrt_ss		_mm_sqrt_ss

	#define reg_cmpeq_ps	_mm_cmpeq_ps
	#define reg_cmpneq_ps	_mm_cmpneq_ps
	#define reg_cmplt_ps	_mm_cmplt_ps
	#define reg_cmpnle_ps	_mm_cmpnle_ps
	#define reg_cmple_ps	_mm_cmple_ps
	#define reg_cmpgt_ps	_mm_cmpgt_ps

	#define reg_cvtepi32_ps	_mm_cvtepi32_ps
	#define reg_cvttss_si32	_mm_cvttss_si32
	#define reg_cvtps_epi32 _mm_cvtps_epi32

#elif USE_NEON > 0
	void _Dummy_() {
		static_assert(false, "not implemented yet");
	}
#else
	#include <cstdint>
	#include <cmath>
	#include <algorithm>
	#include <initializer_list>
	template <class T>
	struct _fullbit {
		static T get() {
			const int c_minus1 = -1;
			return *reinterpret_cast<const T*>(&c_minus1);
		}
	};
	template <class T, int N>
	struct _rbase {
		T v[N];
		_rbase() = default;
		_rbase(const _rbase<T,N>& r) = default;
		template <class T2>
		_rbase(const _rbase<T2,N>& v2) {
			auto* pDst = reinterpret_cast<T*>(v);
			auto* pSrc = reinterpret_cast<const T*>(v2.v);
			for(int i=0 ; i<N ; i++)
				pDst[i] = pSrc[i];
		}
		_rbase(std::initializer_list<T> il) {
			auto* p = v;
			for(auto itr=il.begin() ; itr!=il.end() ; itr++)
				*p++ = (*itr);
		}
		_rbase loadL_PI(const void* src) const {
			auto* pSrc = reinterpret_cast<const T*>(src);
			_rbase ret;
			for(int i=0 ; i<N/2 ; i++)
				ret.v[i] = pSrc[i];
			for(int i=N/2 ; i<N ; i++)
				ret.v[i] = v[i];
			return ret;
		}
		void storePS(T* dst) const {
			for(int i=0 ; i<N ; i++)
				dst[i] = v[i];
		}
		void storeLow(void* dst) const {
			auto* pDst = reinterpret_cast<T*>(dst);
			for(int i=0 ; i<N/2 ; i++)
				pDst[i] = v[i];
		}
		void storeSS(T* dst) const {
			dst[0] = v[0];
		}
		template <int N2, class OP>
		_rbase _operate(const _rbase& r, OP op) const {
			_rbase ret;
			for(int i=0 ; i<N2 ; i++)
				ret.v[i] = op(v[i], r.v[i]);
			for(int i=N2 ; i<N ; i++)
				ret.v[i] = v[i];
			return ret;
		}
		template <int N2, class OP>
		_rbase _operate(OP op) const {
			_rbase ret;
			for(int i=0 ; i<N2 ; i++)
				ret.v[i] = op(v[i]);
			for(int i=N2 ; i<N ; i++)
				ret.v[i] = v[i];
			return ret;
		}

		static T Plus(T t0, T t1) { return t0 + t1; }
		static T Minus(T t0, T t1) { return t0 - t1; }
		static T Mul(T t0, T t1) { return t0 * t1; }
		static T Div(T t0, T t1) { return t0 / t1; }

		static T CmpLT(T t0, T t1) { return t0<t1 ? _fullbit<T>::get() : 0; }
		static T CmpLE(T t0, T t1) { return t0<=t1 ? _fullbit<T>::get() : 0; }
		static T CmpEQ(T t0, T t1) { return t0==t1 ? _fullbit<T>::get() : 0; }
		static T CmpNEQ(T t0, T t1) { return t0!=t1 ? _fullbit<T>::get() : 0; }
		static T CmpGR(T t0, T t1) { return t0>t1 ? _fullbit<T>::get() : 0; }
		static T CmpGE(T t0, T t1) { return t0>=t1 ? _fullbit<T>::get() : 0; }

		static T Min(T t0, T t1) { return t0<t1 ? t0 : t1; }
		static T Max(T t0, T t1) { return t0>t1 ? t0 : t1; }

		#define DEF_REG_OPERATOR(op, func)	_rbase operator op (const _rbase& r) const { return _operate<N>(r, func); }
		DEF_REG_OPERATOR(+, Plus)
		DEF_REG_OPERATOR(-, Minus)
		DEF_REG_OPERATOR(*, Mul)
		DEF_REG_OPERATOR(/, Div)
		DEF_REG_OPERATOR(<, CmpLT)
		DEF_REG_OPERATOR(<=, CmpLE)
		DEF_REG_OPERATOR(==, CmpEQ)
		DEF_REG_OPERATOR(!=, CmpNEQ)
		DEF_REG_OPERATOR(>, CmpGR)
		DEF_REG_OPERATOR(>=, CmpGE)

		_rbase get_min(const _rbase& r) const { return _operate<N>(r, Min); }
		_rbase get_max(const _rbase& r) const { return _operate<N>(r, Max); }
		int32_t movemask() const {
			int32_t res = 0;
			for(int i=0 ; i<N ; i++)
				res |= (*reinterpret_cast<const uint32_t*>(v + i)) >> (31-i);
			return res;
		}

		static T Sqrt(T t) { return std::sqrt(t); }
		static T Rcp(T t) { return T(1) / t; }
		_rbase rcpPS() const {
			return _operate<N>(Rcp);
		}
		_rbase rcpSS() const {
			return _operate<1>(Rcp);
		}
		_rbase sqrtPS() const {
			return _operate<N>(Sqrt);
		}
		_rbase sqrtSS() const {
			return _operate<1>(Sqrt);
		}
		_rbase shuffle(const _rbase& r, int32_t idx) const {
			_rbase ret;
			for(int i=0 ; i<N/2 ; i++,idx>>=2)
				ret.v[i] = v[idx&0x03];
			for(int i=N/2 ; i<N ; i++,idx>>=2)
				ret.v[i] = r.v[idx&0x03];
			return ret;
		}
		_rbase unpackLow(const _rbase& r) const {
			_rbase ret;
			const _rbase *const ptr[2] = {this, &r};
			for(int i=0 ; i<N ; i++)
				ret.v[i] = ptr[i&1]->v[i/2];
			return ret;
		}
		_rbase unpackHigh(const _rbase& r) const {
			_rbase ret;
			const _rbase *const ptr[2] = {this, &r};
			for(int i=0 ; i<N ; i++)
				ret.v[i] = ptr[i&1]->v[(i/2)+(N/2)];
			return ret;
		}
		T& operator [](int n) {
			return v[n];
		}
		template <class T2>
		_rbase& operator = (const _rbase<T2,N>& r) {
			for(int i=0 ; i<N ; i++)
				v[i] = r.v[i];
			return *this;
		}
	};

	#define Operators_Seq (+)(-)(*)(/)(<)(<=)(==)(!=)(>)(>=)
	#define DEF_REG128OP(z, mytype, op)	mytype operator op (const mytype& r) const { \
		return static_cast<const base_t&>(*this) op static_cast<const base_t&>(r); }
	struct reg64i : _rbase<int32_t, 2> {

	};
	struct reg128i : _rbase<int32_t, 4> {
		using base_t = _rbase<int32_t, 4>;
		constexpr static int N = 4;
		using T = int32_t;
		using base_t::base_t;
		using base_t::operator =;

		static T And(T t0, T t1) { return t0 & t1; }
		static T AndNot(T t0, T t1) { return ~t0 & t1; }
		static T Or(T t0, T t1) { return t0 | t1; }
		static T Xor(T t0, T t1) { return t0 ^ t1; }

		DEF_REG_OPERATOR(&, And)
		DEF_REG_OPERATOR(|, Or)
		DEF_REG_OPERATOR(^, Xor)

		BOOST_PP_SEQ_FOR_EACH(DEF_REG128OP, reg128i, Operators_Seq)

		reg128i andnot(const reg128i& r) const { return reg128i(_operate<4>(r, AndNot)); }
		reg128i packS16(const reg128i& r) const {
			reg128i ret;
			auto* dst = reinterpret_cast<int8_t*>(ret.v);
			const reg128i *const ptr[2] = {this, &r};
			for(int i=0 ; i<N*4 ; i++)
				dst[i] = std::max(std::min(ptr[i/(N*2)]->v[i%(N*2)], 0x7f), -0x80);
			return ret;
		}
		reg128i packS32(const reg128i& r) const {
			reg128i ret;
			auto* dst = reinterpret_cast<int16_t*>(ret.v);
			const reg128i *const ptr[2] = {this, &r};
			for(int i=0 ; i<N*2 ; i++)
				dst[i] = std::max(std::min(ptr[i/N]->v[i%N], 0x7fff), -0x8000);
			return ret;
		}
	};
	struct reg128 : _rbase<float, 4> {
		using base_t = _rbase<float, 4>;
		using base_t::base_t;
		using base_t::operator =;

		reg128() = default;
		reg128(const reg128i& r) {
			for(int i=0 ; i<4 ; i++)
				v[i] = r.v[i];
		}

		#define DEF_REG128I_OP(op)  reg128 operator op (const reg128& r) const { \
			reg128i ret = *reinterpret_cast<const reg128i*>(this) op *reinterpret_cast<const reg128i*>(&r); \
			return reinterpret_cast<reg128&>(ret); }
		DEF_REG128I_OP(&)
		DEF_REG128I_OP(|)
		DEF_REG128I_OP(^)
		#undef DEF_REG128I_OP

		BOOST_PP_SEQ_FOR_EACH(DEF_REG128OP, reg128, Operators_Seq)
	};
	struct reg128d : _rbase<double, 2> {};
	#undef DEF_REG128OP
	#undef DEF_REG_OPERATOR
	#undef Operators_Seq

	#define reg_loadu_ps			reg_load_ps
	#define reg_load_ps(p)			(reg128{(p)[0], (p)[1], (p)[2], (p)[3]})
	#define reg_load_ss(p)			(reg128{*(p), 0,0,0})
	#define reg_load_ps1			reg_load1_ps
	#define reg_load1_ps(p)			(reg128{*(p),*(p),*(p),*(p)})
	#define reg_loadl_pi(a,p)		(a).loadL_PI(p)
	#define reg_set_ps(a,b,c,d)		(reg128{(d),(c),(b),(a)})
	#define reg_set1_ps(v)			(reg128{(v),(v),(v),(v)})
	#define reg_setzero_ps()		(reg128{0,0,0,0})
	#define reg_setr_ps(a,b,c,d)	(reg128{(a),(b),(c),(d)})

	#define reg_store_ss(p,r)		(r).storeSS((p))
	#define reg_store_ps(p,r)		(r).storePS((p))
	#define reg_storeu_ps(p,r)		reg_store_ps(p,r)
	#define reg_storel_pi(p,r)		(r).storeLow(p)

	#define reg_add_ps(a,b)			((a) + (b))
	#define reg_sub_ps(a,b)			((a) - (b))
	#define reg_mul_ps(a,b)			((a) * (b))
	#define reg_div_ps(a,b)			((a) / (b))
	#define reg_and_ps(a,b) 		((a) & (b))
	#define reg_or_ps(a,b)			((a) | (b))
	#define reg_xor_ps(a,b)			((a) ^ (b))
	#define reg_andnot_ps(a,b)		(reinterpret_cast<const reg128i*>(&(a))->andnot(*reinterpret_cast<const reg128i*>(&(b))))

	#define reg_move_ss(a,b)		reg128{(b)[0], (a)[1], (a)[2], (a)[3]}
	#define reg_movemask_ps(a)		(a).movemask()
	#define reg_min_ps(a,b)			((a).get_min(b))
	#define reg_max_ps(a,b)			((a).get_max(b))
	#define reg_unpacklo_ps(a,b)	((a).unpackLow(b))
	#define reg_unpackhi_ps(a,b)	((a).unpackHigh(b))
	#define reg_packs_epi32(a,b)	((a).packS32(b))
	#define reg_packus_epi16(a,b)	((a).packS16(b))
	#define reg_shuffle_ps(a,b,idx)	((a).shuffle((b),(idx)))
	#define _REG_SHUFFLE(a,b,c,d)	int32_t((a<<6)|(b<<4)|(c<<2)|(d))

	#define reg_rcp_ps(a)			((a).rcpPS())
	#define reg_sqrt_ps(a)			((a).sqrtPS())
	#define reg_sqrt_ss(a)			((a).sqrtSS())

	#define reg_cmpeq_ps(a,b)		((a) == (b))
	#define reg_cmpneq_ps(a,b)		((a) != (b))
	#define reg_cmplt_ps(a,b)		((a) < (b))
	#define reg_cmpnle_ps(a,b)		((a) > (b))
	#define reg_cmple_ps(a,b)		((a) <= (b))
	#define reg_cmpgt_ps(a,b)		((a) > (b))

	#define reg_cvtepi32_ps(a)		reg128(a)
	#define reg_cvttss_si32(a)		static_cast<int32_t>((a)[0])
	#define reg_cvtps_epi32(a)		reg128i(a)
#endif
