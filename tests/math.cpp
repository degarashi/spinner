#ifdef WIN32
	#include <intrin.h>
#endif
#include "../vector.hpp"
#include "../matrix.hpp"
#include "../matstack.hpp"
#include "../pose.hpp"
#include "../bits.hpp"
#include "../noseq.hpp"
#include "../random.hpp"
#include "../ulps.hpp"
#include "test.hpp"

namespace spn {
	namespace test {
		namespace {
			constexpr float Threshold = 1e-3f,
							ValueMin = -1e4f,
							ValueMax = 1e4f,
							N_Iteration = 100;
			auto OpMul = [](auto& v0, auto& v1){ return v0 * v1; };
			auto OpDiv = [](auto& v0, auto& v1){ return v0 / v1; };
			auto OpAdd = [](auto& v0, auto& v1){ return v0 + v1; };
			auto OpSub = [](auto& v0, auto& v1){ return v0 - v1; };
			auto OpCopy = [](auto& v0, auto& v1){ return v1; };
			template <int M, int N>
			using Array2D = std::array<std::array<float, N>, M>;
			//! 行列と2D配列にそれぞれランダムな値をセット
			template <class RD, class AR, int M, int N, bool A>
			void SetRandom(RD& rd, AR& ar, MatT<M,N,A>& m) {
				for(int i=0 ; i<M ; i++) {
					for(int j=0 ; j<N ; j++) {
						m.ma[i][j] = ar[i][j] = rd();
					}
				}
			}
			//! 行列と2D配列の比較
			template <size_t AM, size_t AN, int M, int N, bool A>
			bool Compare(const std::array<std::array<float,AN>, AM>& ar, const MatT<M,N,A>& m) {
				for(int i=0 ; i<M ; i++) {
					for(int j=0 ; j<N ; j++) {
						auto diff = std::fabs(ar[i][j] - m.ma[i][j]);
						if(diff >= Threshold)
							return false;
					}
				}
				return true;
			}
			//! 行列同士の比較
			template <int M, int N, bool A0, bool A1>
			bool Compare(const MatT<M,N,A0>& m0, const MatT<M,N,A1>& m1) {
				for(int i=0 ; i<M; i++) {
					for(int j=0 ; j<N; j++) {
						auto diff = std::fabs(m0.ma[i][j] - m1.ma[i][j]);
						if(diff >= Threshold)
							return false;
					}
				}
				return true;
			}

			template <class AR>
			struct GetArraySize {
				template <class T, size_t N>
				constexpr static int _size(const std::array<T,N>*) {
					return N;
				}
				constexpr static int size() {
					constexpr AR* ar = nullptr;
					return _size(ar);
				}
			};
			template <class AR, class OP>
			void Operate(AR& ar, OP&& op) {
				using Type = std::decay_t<decltype(ar[0])>;
				for(int i=0 ; i<GetArraySize<AR>::size() ; i++) {
					for(int j=0 ; j<GetArraySize<Type>::size() ; j++)
						op(ar[i][j]);
				}
			}
			template <class AR, class OP>
			void Operate1(AR& ar0, const AR& ar1, OP&& op) {
				using Type = std::decay_t<decltype(ar0[0])>;
				for(int i=0 ; i<GetArraySize<AR>::size(); i++) {
					for(int j=0 ; j<GetArraySize<Type>::size() ; j++)
						ar0[i][j] = op(ar0[i][j], ar1[i][j]);
				}
			}
			template <class AR, class OP>
			void MulArray(AR& ar0, const AR& ar1, OP&& op) {
				using Type = std::decay_t<decltype(ar0[0])>;
				constexpr int width{ GetArraySize<AR>::size() },
							height{ GetArraySize<Type>::size() };
				static_assert(width == height, "rectangular matrix only");
				// サイズ違いの行列を掛けあわせる時は存在しない要素を対角線上は1, それ以外を0とする
				AR tmp;
				for(int i=0 ; i<width ; i++) {
					for(int j=0 ; j<width; j++) {
						tmp[i][j] = 0;
						for(int k=0 ; k<width ; k++)
							tmp[i][j] += ar0[i][k] * ar1[k][j];
					}
				}
				for(int i=0 ; i<width ; i++) {
					for(int j=0 ; j<width ; j++)
						ar0[i][j] = tmp[i][j];
				}
			}
			template <class AR>
			void TransposeArray(AR& ar0, const AR& ar1) {
				using Type = std::decay_t<decltype(ar0[0])>;
				constexpr int width{ GetArraySize<AR>::size() };
				static_assert(width == GetArraySize<Type>::size(), "width must be equal to height");
				for(int i=0 ; i<width ; i++) {
					for(int j=0 ; j<width ; j++)
						ar0[i][j] = ar1[j][i];
				}
			}
			template <class RD, int M, int N, bool A>
			void _MatrixCheckRect(RD& rd,
									std::integral_constant<int,M>,
									std::integral_constant<int,N>,
									std::integral_constant<bool,A>)
			{}
			template <class RD, int N, bool A>
			void _MatrixCheckRect(RD& rd,
									std::integral_constant<int,N>,
									std::integral_constant<int,N>,
									std::integral_constant<bool,A>)
			{
				using Mat = MatT<N,N,A>;
				auto fnTestMat = [&rd](auto&& opA, auto&& opM) {
					for(int i=0 ; i<N_Iteration ; i++) {
						Mat m0, m1;
						Array2D<N,N> ar0, ar1;
						SetRandom(rd, ar0, m0);
						SetRandom(rd, ar1, m1);
						opA(ar0, ar1, std::forward<std::decay_t<decltype(opM)>>(opM));
						auto m2 = opM(m0,m1);
						m0 = opM(m0, m1);
						EXPECT_TRUE(Compare(ar0, m0));
					}
				};
				// + mat
				fnTestMat([](auto& a0, auto& a1, auto&&){ Operate1(a0,a1,OpAdd); }, OpAdd);
				// - mat
				fnTestMat([](auto& a0, auto& a1, auto&&){ Operate1(a0,a1,OpSub); }, OpSub);
				// * mat
				auto* ptr = &MulArray<Array2D<N,N>, decltype(OpMul)>;
				fnTestMat(ptr, OpMul);
				// = mat
				fnTestMat([](auto& a0, auto& a1, auto&&){ Operate1(a0,a1,OpCopy); }, OpCopy);
				// determinant
				// inversion

				// transpose
				Array2D<N,N>	ar0,ar1;
				Mat				m0,m1;
				SetRandom(rd, ar1, m1);
				TransposeArray(ar0, ar1);
				m0 = m1.transposition();
				EXPECT_TRUE(Compare(ar0, m0));
			}
			template <int M, int N, bool A>
			void _MatrixCheck(MTRandom& rd) {
				// 配列を使って計算した結果とMatrixクラスの結果を、誤差含めて比較
				auto rdF = [&rd](){ return rd.template getUniformRange<float>(ValueMin, ValueMax); };

				using Mat = MatT<M,N,A>;
				auto fnTestScalar = [&rdF](auto& rd, auto&& op) {
					for(int i=0 ; i<N_Iteration ; i++) {
						Mat m0;
						Array2D<M,N> ar0;
						SetRandom(rdF, ar0, m0);

						float value = rd();
						Operate(ar0, [value, &op](auto& v){ v = op(v, value); });
						m0 = op(m0, value);
						EXPECT_TRUE(Compare(ar0, m0));
					}
				};
				// += scalar
				fnTestScalar(rdF, OpAdd);
				// -= scalar
				fnTestScalar(rdF, OpSub);
				// *= scalar
				fnTestScalar(rdF, OpMul);
				// /= scalar
				fnTestScalar(rdF, OpDiv);
				// --- Rect Only ---
				_MatrixCheckRect(rdF,
								std::integral_constant<int,M>(),
								std::integral_constant<int,N>(),
								std::integral_constant<bool,A>());
			}
			template <int M, int N>
			void MatrixCheck(MTRandom& rd) {
				_MatrixCheck<M,N,false>(rd);
				_MatrixCheck<M,N,true>(rd);
			}
		}
		class MathTest : public RandomTestInitializer {};
		#define MATRIXCHECK(z, dummy, dim)	MatrixCheck<BOOST_PP_TUPLE_ELEM(0,dim), BOOST_PP_TUPLE_ELEM(1,dim)>(rd);
		TEST_F(MathTest, Matrix) {
			auto rd = getRand();
			BOOST_PP_SEQ_FOR_EACH(MATRIXCHECK, BOOST_PP_IDENTITY, SEQ_MATDEF)
		}
		#undef MATRIXCHECK

		namespace {
			template <int N, bool A, class RD>
			auto GenRVec(RD& rd) {
				VecT<N,A> v;
				for(int i=0 ; i<N ; i++)
					v.m[i] = rd();
				return v;
			}
			template <int N, bool A, class RD>
			auto GenRVecNZ(RD& rd, float th) {
				VecT<N,A> v;
				do {
					v = GenRVec<N,A>(rd);
				} while(v.length() < th);
				return v;
			}
			template <int N, bool A, class RD>
			auto GenRDir(RD& rd) {
				return GenRVecNZ<N,A>(rd, 1e-4f).normalization();
			}
			template <bool A, class RD>
			auto GenRQuat(RD& rd) {
				return QuatT<A>::Rotation(GenRDir<3,A>(rd), rd());
			}
		}
		// AlignedとUnAlignedを両方テストする
		template <class T>
		class QuaternionTest : public RandomTestInitializer {
			protected:
				using QuatType = QuatT<T::value>;
		};
		using QuaternionTypeList = ::testing::Types<std::integral_constant<bool,false>,
										std::integral_constant<bool,true>>;
		TYPED_TEST_CASE(QuaternionTest, QuaternionTypeList);
		TYPED_TEST(QuaternionTest, Test) {
			using QT = typename std::decay_t<decltype(*this)>::QuatType;
			constexpr float RandMin = -1e3f,
							RandMax = 1e3f;
			auto rd = this->getRand();
			auto rdF = [RandMin, RandMax, &rd](){ return rd.template getUniformRange<float>(RandMin, RandMax); };

			for(int i=0 ; i<N_Iteration ; i++) {
				float ang = rdF();
				Vec3 axis = GenRDir<3,false>(rdF);
				QT q = QT::Rotation(axis, ang);
				auto m = AMat33::RotationAxis(axis, ang);

				// クォータニオンでベクトルを変換した結果が行列のそれと一致するか
				Vec3 v = GenRVec<3,false>(rdF);
				auto v0 = v * q;
				auto v1 = v * m;
				EXPECT_TRUE(EqAbs(v0, v1, Threshold));
				// クォータニオンを行列に変換した結果が一致するか
				EXPECT_TRUE(Compare(q.asMat33(), m));
				// Matrix -> Quaternion -> Matrix の順で変換して前と後で一致するか
				q = QT::FromMat(m);
				EXPECT_TRUE(Compare(q.asMat33(), m));
			}
			// クォータニオンを合成した結果を行列のケースと比較
			for(int i=0 ; i<N_Iteration ; i++) {
				float ang[2] = {rdF(), rdF()};
				Vec3 axis[2] = {GenRDir<3,false>(rdF), GenRDir<3,false>(rdF)};
				QT		q[3];
				AMat33	m[3];
				for(int i=0 ; i<2 ; i++) {
					q[i] = QT::Rotation(axis[i], ang[i]);
					m[i] = AMat33::RotationAxis(axis[i], ang[i]);
				}
				q[2] = q[1] * q[0];
				q[2].normalize();
				m[2] = m[0] * m[1];
				EXPECT_TRUE(Compare(q[2].asMat33(), m[2]));
				// operator >> は * と逆の意味
				q[2] = q[0] >> q[1];
				q[2].normalize();
				EXPECT_TRUE(Compare(q[2].asMat33(), m[2]));
			}
			// getRight(), getUp(), getDir()が{1,0,0},{0,1,0},{0,0,1}を変換した結果と比較
			for(int i=0 ; i<N_Iteration ; i++) {
				float ang = rdF();
				Vec3 axis = GenRDir<3,false>(rdF);
				QT q = QT::Rotation(axis, ang);
				auto m = q.asMat33();

				EXPECT_TRUE(EqAbs(AVec3{1,0,0}*m, q.getRight(), Threshold));
				EXPECT_TRUE(EqAbs(AVec3{0,1,0}*m, q.getUp(), Threshold));
				EXPECT_TRUE(EqAbs(AVec3{0,0,1}*m, q.getDir(), Threshold));
			}
		}
		TEST_F(MathTest, Pose) {
			constexpr float RandMin = -1e3f,
							RandMax = 1e3f;
			auto rd = getRand();
			auto rdF = [RandMin, RandMax, &rd](){ return rd.template getUniformRange<float>(RandMin, RandMax); };

			// DecompAffineした結果を再度合成して同じかどうかチェック
			for(int i=0 ; i<N_Iteration ; i++) {
				auto q = GenRQuat<false>(rdF);
				auto t = GenRVec<3,false>(rdF);
				auto s = GenRVecNZ<3,false>(rdF, 1e-2f);
				s = Vec3{-1,1,1};

				Pose3D pose(t, q, s);
				auto m = pose.getToWorld();
				auto ap = DecompAffine(m);
				EXPECT_TRUE(EqAbs(t, ap.offset, Threshold));
				EXPECT_TRUE(EqAbs(AVec3{1,0,0}*ap.rotation, q.getRight(), Threshold));
				EXPECT_TRUE(EqAbs(AVec3{0,1,0}*ap.rotation, q.getUp(), Threshold));
				EXPECT_TRUE(EqAbs(AVec3{0,0,1}*ap.rotation, q.getDir(), Threshold));
				EXPECT_TRUE(EqAbs(s, ap.scale, Threshold));
			}
		}
		// TODO: Pose::lerp チェック
		// TODO: Vec::FromPacked チェック
		// TODO: noseq チェック
		// TODO: Assert文の追加
		TEST_F(MathTest, Bitfield) {
			struct MyDef : BitDef<uint32_t, BitF<0,14>, BitF<14,6>, BitF<20,12>> {
				enum { VAL0, VAL1, VAL2 }; };
			using Value = BitField<MyDef>;
			Value value(0);
			value.at<Value::VAL2>() = ~0;
			auto mask = value.mask<Value::VAL1>();
			auto raw = value.value();

			std::cout << std::hex << "raw:\t" << raw << std::endl
									<< "mask:\t" << mask << std::endl;
		}

		namespace {
			template <class T>
			void TestAsIntegral(MTRandom rd) {
				auto fvalue = rd.getUniformRange<T>(std::numeric_limits<T>::lowest()/2,
														std::numeric_limits<T>::max()/2);
				auto ivalue0 = AsIntegral(fvalue);
				auto ivalue1 = *reinterpret_cast<decltype(ivalue0)*>(&fvalue);
				EXPECT_EQ(ivalue0, ivalue1);
			}
		}
		TEST_F(MathTest, FloatAsIntegral) {
			TestAsIntegral<float>(getRand());
		}
		TEST_F(MathTest, DoubleAsIntegral) {
			TestAsIntegral<double>(getRand());
		}

		TEST_F(MathTest, GapStructure) {
			auto rd = getRand();
			auto gen = std::bind(&MTRandom::getUniform<float>, std::ref(rd));
			using F16 = std::array<float,16>;
			F16 tmp;
			std::generate(tmp.begin(), tmp.end(), std::ref(gen));

			#define SETAUX(z,n,data)	aux##n = *data++;
			#define CHECKAUX(z,n,data) EXPECT_FLOAT_EQ(aux##n, *data++);
			{
				struct Test {
					GAP_MATRIX(mat, 4,3,
							(((float, aux0)))
							(((float, aux1)))
							(((float, aux2)))
							(((float, aux3)))
					)

					Test(const F16& tmp) {
						auto* src = SetValue(mat, &tmp[0]);
						BOOST_PP_REPEAT(4, SETAUX, src)
						src = CheckValue(mat, &tmp[0]);
						BOOST_PP_REPEAT(4, CHECKAUX, src)
					}
				} test(tmp);
			}
			{
				struct Test {
					GAP_MATRIX(mat, 4,2,
							(((float, aux0), (float, aux4)))
							(((float, aux1), (float, aux5)))
							(((float, aux2), (float, aux6)))
							(((float, aux3), (float, aux7)))
					)
					Test(const F16& tmp) {
						auto* src = SetValue(mat, &tmp[0]);
						BOOST_PP_REPEAT(8, SETAUX, src)
						src = CheckValue(mat, &tmp[0]);
						BOOST_PP_REPEAT(8, CHECKAUX, src)
					}
				} test(tmp);
			}
			{
				struct Test {
					GAP_VECTOR(vec, 3, (float aux0))
					Test(const F16& tmp) {
						auto* src = SetValue(vec, &tmp[0]);
						BOOST_PP_REPEAT(1, SETAUX, src)
						src = CheckValue(vec, &tmp[0]);
						BOOST_PP_REPEAT(1, CHECKAUX, src)
					}
				} test(tmp);
			}
			{
				struct Test {
					GAP_VECTOR(vec, 2, (float aux0)(float aux1))
					Test(const F16& tmp) {
						auto* src = SetValue(vec, &tmp[0]);
						BOOST_PP_REPEAT(2, SETAUX, src)
						src = CheckValue(vec, &tmp[0]);
						BOOST_PP_REPEAT(2, CHECKAUX, src)
					}
				} test(tmp);
			}
			#undef SETAUX
			#undef CHECKAUX
		}
	}
}
