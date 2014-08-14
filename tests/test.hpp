#pragma once
#ifdef WIN32
	#include <intrin.h>
#endif
#include "../matrix.hpp"
#include "../random.hpp"
#include "../path.hpp"
#include <type_traits>
#include <gtest/gtest.h>

//TODO reg128のテスト
namespace spn {
	namespace test {
		//! ランダムなベクトル
		template <int N, bool A, class RD>
		auto GenRVec(RD& rd) {
			VecT<N,A> v;
			for(int i=0 ; i<N ; i++)
				v.m[i] = rd();
			return v;
		}
		//! ランダムなベクトル（但しゼロではない）
		template <int N, bool A, class RD>
		auto GenRVecNZ(RD& rd, float th) {
			VecT<N,A> v;
			do {
				v = GenRVec<N,A>(rd);
			} while(v.length() < th);
			return v;
		}
		//! ランダムな方向ベクトル
		template <int N, bool A, class RD>
		auto GenRDir(RD& rd) {
			return GenRVecNZ<N,A>(rd, 1e-4f).normalization();
		}
		//! ランダムなクォータニオン
		template <bool A, class RD>
		auto GenRQuat(RD& rd) {
			return QuatT<A>::Rotation(GenRDir<3,A>(rd), rd());
		}
		template <bool A, class RD>
		auto GenRExpQuat(RD& rd) {
			return ExpQuatT<A>(GenRQuat<A>(rd));
		}
		template <bool A, int M, int N, class RD>
		auto GenRMat(RD& rd) {
			MatT<M,N,A> m;
			for(int i=0 ; i<M ; i++) {
				for(int j=0 ; j<N ; j++)
					m.ma[i][j] = rd();
			}
			return m;
		}

		extern PathBlock g_apppath;
		template <int M, int N>
		const float* SetValue(MatT<M,N,true>& m, const float* src) {
			for(int i=0 ; i<M ; i++)
				for(int j=0 ; j<N ; j++)
					m.ma[i][j] = *src++;
			return src;
		}
		template <int M, int N>
		const float* CheckValue(MatT<M,N,true>& m, const float* src) {
			for(int i=0 ; i<M ; i++)
				for(int j=0 ; j<N ; j++)
					EXPECT_FLOAT_EQ(m.ma[i][j], *src++);
			return src;
		}
		template <int N>
		const float* SetValue(VecT<N,true>& v, const float* src) {
			for(int i=0 ; i<N ; i++)
				v.m[i] = *src++;
			return src;
		}
		template <int N>
		const float* CheckValue(VecT<N,true>& v, const float* src) {
			for(int i=0 ; i<N ; i++)
				EXPECT_FLOAT_EQ(v.m[i], *src++);
			return src;
		}
		//! non-copyableな値
		template <class T>
		struct MoveOnly {
			using move_type = T;
			T	value;

			MoveOnly(const T& v): value(v) {}
			MoveOnly(T&& v): value(std::move(v)) {}
			MoveOnly(const MoveOnly&) = delete;
			MoveOnly(MoveOnly&& m): value(std::move(m.value)) {}
			void operator = (const MoveOnly&) = delete;
			MoveOnly& operator = (MoveOnly&& m) {
				value = std::move(m.value);
				return *this;
			}
			template <class T2>
			static const T2& _getValue(const T2& t, ...) {
				return t;
			}
			template <class T3>
			static auto _getValue(const MoveOnly<T3>& t, ...) -> decltype(t.getValue()){
				return t.getValue();
			}
			auto getValue() const -> decltype(_getValue(value, nullptr)) {
				return _getValue(value, nullptr);
			}
			bool operator == (const MoveOnly& m) const {
				return getValue() == m.getValue();
			}
			bool operator < (const MoveOnly& m) const {
				return getValue() < m.getValue();
			}
		};

		void PrintReg128(std::ostream& os, reg128 r);
		class RandomTestInitializer : public ::testing::Test {
			public:
				constexpr static MTRandomMgr::ID cs_rnd = 0xff;
			protected:
				void SetUp() override;
				void TearDown() override;
				MTRandom getRand();
		};

		#define SEQ_VECTOR (2)(3)(4)
		#define VECTOR_TYPES(z, align, elem) \
			(integralI_Vec<(elem << 4) | align>)

		// ベクトルの全パターン組み合わせ
		/*	NUM & 0x01 = Align
			NUM & 0xf0 = N */
		template <int N>
		using integralI_Vec = VecT<((N&0xf0)>>4), N&0x01>;
		using VecTP = ::testing::Types<
			BOOST_PP_SEQ_ENUM(
				BOOST_PP_SEQ_FOR_EACH(VECTOR_TYPES, 0x00, SEQ_VECTOR)
				BOOST_PP_SEQ_FOR_EACH(VECTOR_TYPES, 0x01, SEQ_VECTOR)
			)
		>;
		#undef SEQ_VECTOR
		#undef VECTOR_TYPES

		// 行列の全パターン組み合わせ
		/*	NUM & 0x001 = Align
			NUM & 0xf00 = M
			NUM & 0x0f0 = N */
		template <int N>
		using integralI_Mat = MatT<((N&0xf00)>>8), ((N&0x0f0)>>4), N&0x001>;
		#define MATRIX_TYPES(z, align, elem) \
			(integralI_Mat<(BOOST_PP_TUPLE_ELEM(0,elem) << 8) | (BOOST_PP_TUPLE_ELEM(1,elem) << 4) | align>)
		using MatTP = ::testing::Types<
			BOOST_PP_SEQ_ENUM(
				BOOST_PP_SEQ_FOR_EACH(MATRIX_TYPES, 0x00, SEQ_MATDEF)
				BOOST_PP_SEQ_FOR_EACH(MATRIX_TYPES, 0x01, SEQ_MATDEF)
			)
		>;
		#undef MATRIX_TYPES
	}
}

