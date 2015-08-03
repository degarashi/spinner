#pragma once
#ifdef WIN32
	#include <intrin.h>
#endif
#include "../matrix.hpp"
#include "../random.hpp"
#include "../path.hpp"
#include "../ulps.hpp"
#include "../check_macro.hpp"
#include "../pose.hpp"
#include <type_traits>
#include <gtest/gtest.h>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/utility.hpp>

namespace boost {
	namespace archive {
		class text_oarchive;
		class text_iarchive;
		class binary_oarchive;
		class binary_iarchive;
		class xml_oarchive;
		class xml_iarchive;
	}
}
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
		namespace {
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wunused-function"
			namespace test_i {
				DEF_HASTYPE(width)
				template <class T, std::enable_if_t<!std::is_floating_point<T>::value>*& = Enabler>
				void _CheckIt(const T& v0, const T& v1, std::false_type) {
					EXPECT_EQ(v0, v1);
				}
				constexpr float c_ulps(ULPs_C(1.0f, 1.0f+1e-6f));
				// (テキストにfloating pointを保存する場合は細かい値が変動する場合がある為)
				template <class T, std::enable_if_t<std::is_floating_point<T>::value>*& = Enabler>
				void _CheckIt(const T& v0, const T& v1, std::false_type) {
					EXPECT_TRUE(EqULPs(v0, v1, c_ulps));
				}
				template <class T>
				void _CheckIt(const T& v0, const T& v1, std::true_type) {
					EXPECT_TRUE(EqULPs(v0, v1, c_ulps));
				}
				template <class T>
				void CheckIt(const T& v0, const T& v1) {
					_CheckIt(v0, v1, HasType_width<T>(nullptr));
				}
				void CheckIt(const Pose3D& v0, const Pose3D& v1) {
					CheckIt(v0.getOffset(), v1.getOffset());
					CheckIt(v0.getRot(), v1.getRot());
					CheckIt(v0.getScale(), v1.getScale());
				}
				void CheckIt(const Pose2D& v0, const Pose2D& v1) {
					CheckIt(v0.getOffset(), v1.getOffset());
					CheckIt(v0.getAngle(), v1.getAngle());
					CheckIt(v0.getScale(), v1.getScale());
				}
			}
			#pragma GCC diagnostic pop
		}
		template <class OA, class IA, class T>
		void CheckSerializedData(const T& src) {
			std::stringstream buffer;
			OA oa(buffer);
			oa << BOOST_SERIALIZATION_NVP(src);
			T loaded;
			IA ia(buffer);
			ia >> boost::serialization::make_nvp("src", loaded);
			test_i::CheckIt(src, loaded);
		}
		namespace test_i {}

		template <class T>
		void CheckSerializedDataBin(const T& src) {
			CheckSerializedData<boost::archive::binary_oarchive,
								boost::archive::binary_iarchive>(src);
		}
		template <class T>
		void CheckSerializedDataText(const T& src) {
			CheckSerializedData<boost::archive::text_oarchive,
								boost::archive::text_iarchive>(src);
		}
		template <class T>
		void CheckSerializedDataXML(const T& src) {
			CheckSerializedData<boost::archive::xml_oarchive,
								boost::archive::xml_iarchive>(src);
		}

		//! non-copyableな値
		template <class T>
		struct MoveOnly {
			using move_type = T;
			T	value;
			template <class Ar>
			void serialize(Ar& /*ar*/, const unsigned int) {}

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
			bool operator != (const MoveOnly& m) const {
				return !(this->operator == (m));
			}
			bool operator < (const MoveOnly& m) const {
				return getValue() < m.getValue();
			}
			bool operator > (const MoveOnly& m) const {
				return getValue() > m.getValue();
			}
			const T& get() const {
				return value;
			}
		};
		template <class T>
		bool operator == (const T& value, const MoveOnly<T>& value1) noexcept { return value == value1.get(); }
		template <class T>
		bool operator == (const MoveOnly<T>& value, const T& value1) noexcept { return value.get() == value1; }
		template <class T>
		bool operator != (const T& value, const MoveOnly<T>& value1) noexcept { return value != value1.get(); }
		template <class T>
		bool operator != (const MoveOnly<T>& value, const T& value1) noexcept { return value.get() != value1; }
		template <class T>
		std::ostream& operator << (std::ostream& os, const MoveOnly<T>& value) {
			return os << value.getValue();
		}

		void PrintReg128(std::ostream& os, reg128 r);
		namespace {
			namespace test_i2 {
				template <class BASE>
				class RandomTestInitializer : public BASE {
					public:
						constexpr static MTRandomMgr::ID cs_rnd = 0xff;
					protected:
						void SetUp() override {
							mgr_random.initEngine(cs_rnd);
						}
						void TearDown() override {
							mgr_random.removeEngine(cs_rnd);
						}
						MTRandom getRand() {
							return mgr_random.get(cs_rnd);
						}
				};
			}
		}
		template <class T>
		class RandomTestInitializerP : public test_i2::RandomTestInitializer<::testing::TestWithParam<T>> {};
		class RandomTestInitializer : public test_i2::RandomTestInitializer<::testing::Test> {};
		namespace test_i2 {}

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
namespace boost {
	namespace serialization {
		template <class Ar, class T>
		inline void load_construct_data(Ar& ar, spn::test::MoveOnly<T>* mv, const unsigned int) {
			T value;
			ar	& boost::serialization::make_nvp("value", value);
			new(mv) spn::test::MoveOnly<T>(std::move(value));
		}
		template <class Ar, class T>
		inline void save_construct_data(Ar& ar, const spn::test::MoveOnly<T>* mv, const unsigned int) {
			ar	& boost::serialization::make_nvp("value", mv->value);
		}
	}
}

