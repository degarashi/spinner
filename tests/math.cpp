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
#include "../expquat.hpp"
#include "test.hpp"

namespace spn {
	namespace test {
		namespace {
			constexpr auto ThresholdULPs = ULPs_C(0.f, 1e-4f);
			constexpr float ValueMin = -1e4f,
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
						if(!EqULPs(ar[i][j], m.ma[i][j], ThresholdULPs))
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
						if(!EqULPs(m0.ma[i][j], m1.ma[i][j], ThresholdULPs))
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
		}
		// AlignedとUnAlignedを両方テストする
		template <class T>
		class QuaternionTest : public RandomTestInitializer {
			protected:
				using QuatType = QuatT<T::value>;
				constexpr static bool Align = T::value;
				const static float RandMin,
									RandMax;
				Optional<MTRandom>		_rd;
				std::function<float ()>	_fnRandF,
										_fnRandF01,
										_fnRandPI;

				void SetUp() override {
					RandomTestInitializer::SetUp();
					_rd = getRand();
					_fnRandF = [&rd=*_rd](){ return rd.template getUniformRange<float>(RandMin, RandMax); };
					_fnRandF01 = [&rd=*_rd](){ return rd.template getUniformRange<float>(0,1); };
					_fnRandPI = [&rd=*_rd](){ return rd.template getUniformRange<float>(-PI,PI); };
				}
				void TearDown() override {
					_rd = none;
					RandomTestInitializer::TearDown();
				}
				MTRandom& refRand() { return _rd; }
				auto getRandPI() { return _fnRandPI; }
				auto getRandF() { return _fnRandF; }
				auto getRandF01() { return _fnRandF01; }
				QuatType genRandQ() { return GenRQuat<Align>(_fnRandF); }
				auto genRandDir() { return GenRDir<3,Align>(_fnRandF); }
		};
		template <class T>
		const float QuaternionTest<T>::RandMin = -1e3f;
		template <class T>
		const float QuaternionTest<T>::RandMax = 1e3f;

		using QuaternionTypeList = ::testing::Types<std::integral_constant<bool,false>,
										std::integral_constant<bool,true>>;
		TYPED_TEST_CASE(QuaternionTest, QuaternionTypeList);

		template <class T>
		class ExpQuaternionTest : public QuaternionTest<T> {
			protected:
				using base_t = QuaternionTest<T>;
				using ExpType = ExpQuatT<T::value>;
				ExpType genRandEQ() { return GenRExpQuat(base_t::_fnRandF); }
		};
		using ExpQuaternionTypeList = QuaternionTypeList;
		TYPED_TEST_CASE(ExpQuaternionTest, ExpQuaternionTypeList);

		TYPED_TEST(ExpQuaternionTest, QuaternionConvert) {
			using This_t = std::decay_t<decltype(*this)>;
			using EQ = typename This_t::ExpType;

			// Quat -> ExpQuat -> Quat の結果比較
			for(int i=0 ; i<N_Iteration ; i++) {
				auto q0 = this->genRandQ();
				EQ eq(q0);
				auto q1 = eq.asQuat();
				auto aa0 = eq.getAngAxis();
				eq = EQ(q1*-1);
				auto aa1 = eq.getAngAxis();
				auto q2 = eq.asQuat();
				// クォータニオンの符号反転した物は同一視 = 方向ベクトルを変換して同じだったらOK
				auto v0 = this->genRandDir();
				auto v1 = v0 * q0,
					v2 = v0 * q1,
					v3 = v0 * q2;
				EXPECT_TRUE(EqAbs(v1, v2, 5e-2f));
				EXPECT_TRUE(EqAbs(v1, v3, 5e-2f));
			}
		}
		TYPED_TEST(ExpQuaternionTest, Lerp) {
			using This_t = std::decay_t<decltype(*this)>;
			using EQ = typename This_t::ExpType;
			auto rdF01 = this->getRandF01();

			// ExpQuatを合成した結果をQuat合成のケースと比較
			for(int i=0 ; i<N_Iteration ; i++) {
				using QT = typename This_t::QuatType;
				auto q0 = this->genRandQ(),
					q1 = this->genRandQ();
				// 最短距離で補間するような細工
				if(q0.dot(q1) < 0)
					q1 *= -1;
				EQ eq0(q0),
					eq1(q1),
					eq10(q0 * -1),
					eq11(q1 * -1);
				auto len0 = eq0.asVec3().length(),
					len1 = eq1.asVec3().length(),
					len10 = eq10.asVec3().length(),
					len11 = eq11.asVec3().length();
				if(len0+len1 > len10+len11) {
					eq0 = q0 * -1;
					eq1 = q1 * -1;
					eq10 = q0;
					eq11 = q1;
				}

				constexpr int NDiv = 32;
				float err_sum = 0;
				auto v0 = this->genRandDir();
				// 0から1まで遷移
				for(int i=0 ; i<=NDiv ; i++) {
					float t = i/float(NDiv);
					// Quatで変換
					auto v1 = v0 * q0.slerp(q1, t);
					// ExpQuatで変換
					auto eq2 = eq0 * (1.f-t) + eq1 * t;
					auto v2 = v0 * eq2.asQuat();

					auto diff = v2-v1;
					err_sum += diff.dot(diff);
				}
				err_sum /= NDiv+1;
				EXPECT_LE(err_sum, 0.36f);
			}
		}
		TYPED_TEST(QuaternionTest, ConvertMatrix) {
			using This_t = std::decay_t<decltype(*this)>;
			constexpr bool Align = This_t::Align;
			using QT = typename This_t::QuatType;
			auto rdF = this->getRandF();

			for(int i=0 ; i<N_Iteration ; i++) {
				float ang = rdF();
				Vec3 axis = GenRDir<3,false>(rdF);
				QT q = QT::Rotation(axis, ang);
				auto m = AMat33::RotationAxis(axis, ang);

				// クォータニオンでベクトルを変換した結果が行列のそれと一致するか
				auto v = GenRVec<3,Align>(rdF);
				auto v0 = v * q;
				auto v1 = v * m;
				EXPECT_TRUE(EqULPs(v0, v1, ThresholdULPs));
				// クォータニオンを行列に変換した結果が一致するか
				EXPECT_TRUE(Compare(q.asMat33(), m));
				// Matrix -> Quaternion -> Matrix の順で変換して前と後で一致するか
				q = QT::FromMat(m);
				EXPECT_TRUE(Compare(q.asMat33(), m));
			}
		}
		TYPED_TEST(QuaternionTest, Multiply) {
			using This_t = std::decay_t<decltype(*this)>;
			using QT = typename This_t::QuatType;
			auto rdF = this->getRandF();

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
		}
		TYPED_TEST(QuaternionTest, Rotation) {
			using This_t = std::decay_t<decltype(*this)>;
			constexpr bool Align = This_t::Align;
			using QT = typename This_t::QuatType;
			auto rdF = this->getRandF();

			// getRight(), getUp(), getDir()が{1,0,0},{0,1,0},{0,0,1}を変換した結果と比較
			for(int i=0 ; i<N_Iteration ; i++) {
				float ang = rdF();
				Vec3 axis = GenRDir<3,false>(rdF);
				QT q = QT::Rotation(axis, ang);
				auto m = q.asMat33();

				EXPECT_TRUE(EqULPs(VecT<3,Align>(Vec3{1,0,0}*m), q.getRight(), ThresholdULPs));
				EXPECT_TRUE(EqULPs(VecT<3,Align>(AVec3{0,1,0}*m), q.getUp(), ThresholdULPs));
				EXPECT_TRUE(EqULPs(VecT<3,Align>(AVec3{0,0,1}*m), q.getDir(), ThresholdULPs));
			}
		}
		TYPED_TEST(QuaternionTest, SLerp) {
			using This_t = std::decay_t<decltype(*this)>;
			using QT = typename This_t::QuatType;
			auto rdPI = this->getRandPI();

			// クォータニオンの線形補間
			for(int i=0 ; i<N_Iteration ; i++) {
				const int div = 32;
				float tdiv = 1.f/div;
				auto axis = this->genRandDir();
				auto ang = rdPI();
				auto q0 = this->genRandQ();
				auto q1 = QT::Rotation(axis, ang);
				q1 = q0 >> q1;
				Mat33 m0 = q0.asMat33();
				for(int i=0 ; i<div ; i++) {
					float t = tdiv * i;
					Mat33 m1 = m0 * Mat33::RotationAxis(axis, ang*t);
					auto q2 = q0.slerp(q1, t);

					auto v = this->genRandDir();
					auto v0 = v * q2;
					auto v1 = v * m1;
					EXPECT_TRUE(EqULPs(v0, v1, ThresholdULPs));
				}
			}
		}
		TEST_F(MathTest, DecompAffine) {
			constexpr float RandMin = -1e3f,
							RandMax = 1e3f;
			auto rd = getRand();
			auto rdF = [RandMin, RandMax, &rd](){ return rd.template getUniformRange<float>(RandMin, RandMax); };

			// 行列をDecompAffineした結果を再度合成して同じかどうかチェック
			for(int i=0 ; i<N_Iteration ; i++) {
				auto q = GenRQuat<true>(rdF);
				auto t = GenRVec<3,true>(rdF);
				auto s = GenRVecNZ<3,true>(rdF, 1e-2f);

				Pose3D pose(t, q, s);
				auto m = pose.getToWorld();
				auto ap = DecompAffine(m);
				auto m2 = Pose3D(ap.offset, ap.rotation, ap.scale).getToWorld();
				EXPECT_TRUE(EqULPs(m, m2, ThresholdULPs));
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
				auto ivalueC = AsIntegral_C(fvalue);
				auto ivalue = AsIntegral(fvalue);
				EXPECT_EQ(ivalueC, ivalue);
			}
			template <class T>
			void TestCompare(MTRandom rd) {
				auto fnRand = [&rd]() {
					auto range0 = static_cast<T>(std::pow(T(2), std::numeric_limits<T>::max_exponent/2));
					return rd.getUniformRange<T>(-range0, range0);
				};
				for(int i=0 ; i<N_Iteration ; i++) {
					auto f0 = fnRand(),
						f1 = fnRand();
					// ULPs単位でどの程度離れているか
					auto ulps = ULPs(f0, f1),
						ulps_1 = std::max(decltype(ulps)(0), ulps-1);
					if(ulps < 0) {
						--i;
						continue;
					}
					// Equal範囲チェック
					EXPECT_TRUE(EqULPs(f0, f1, ulps));
					if(ulps > 0)
						EXPECT_FALSE(EqULPs(f0, f1, ulps_1));

					// LessEqual範囲チェック
					EXPECT_TRUE(LeULPs(f0, f1, ulps));
					if(ulps > 0) {
						if(f0 < f1)
							EXPECT_TRUE(LeULPs(f0, f1, ulps_1));
						else
							EXPECT_FALSE(LeULPs(f0, f1, ulps_1));
					}

					// GreaterEqual範囲チェック
					EXPECT_TRUE(GeULPs(f0, f1, ulps));
					if(ulps > 0) {
						if(f0 > f1)
							EXPECT_TRUE(GeULPs(f0, f1, ulps_1));
						else
							EXPECT_FALSE(GeULPs(f0, f1, ulps_1));
					}
				}
			}
		}
		TEST_F(MathTest, FloatAsIntegral) {
			TestAsIntegral<float>(getRand());
		}
		TEST_F(MathTest, DoubleAsIntegral) {
			TestAsIntegral<double>(getRand());
		}
		TEST_F(MathTest, FloatCompareULPs) {
			TestCompare<float>(getRand());
		}
		TEST_F(MathTest, DoubleCompareULPs) {
			TestCompare<double>(getRand());
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
