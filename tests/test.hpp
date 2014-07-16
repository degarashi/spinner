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
	}
}

