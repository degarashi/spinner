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
#include "../structure/angle.hpp"
#include "test.hpp"

#pragma GCC diagnostic ignored "-Wunused-variable"
namespace spn {
	namespace test {
		namespace {
			constexpr auto ThresholdULPs = ULPs_C(0.f, 3e-4f);
			constexpr auto ThresholdULPs_Quat = ULPs_C(0.f, 5e-3f);
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
				auto rdF = [&rd](){ return rd.template getUniform<float>({ValueMin, ValueMax}); };

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

		// AlignedとUnAlignedを両方テストする
		template <class T>
		class QuaternionTest : public RandomTestInitializer {
			protected:
				using QuatType = QuatT<T::value>;
				constexpr static bool Align = T::value;
				Optional<MTRandom>		_rd;

				void SetUp() override {
					RandomTestInitializer::SetUp();
					_rd = getRand();
				}
				void TearDown() override {
					_rd = none;
					RandomTestInitializer::TearDown();
				}
				MTRandom& refRand() { return *_rd; }
				QuatType genRandQ() { return QuatT<Align>::Random(refRand().template getUniformF<float>()); }
				auto genRandDir() { return VecT<3,Align>::RandomDir(refRand().template getUniformF<float>()); }
		};
		using QuaternionTypeList = ::testing::Types<std::false_type, std::true_type>;
		TYPED_TEST_CASE(QuaternionTest, QuaternionTypeList);

		template <class T>
		class ExpQuaternionTest : public QuaternionTest<T> {
			protected:
				using base_t = QuaternionTest<T>;
				using ExpType = ExpQuatT<T::value>;
				ExpType genRandEQ() { return ExpType::Random(base_t::refRand()); }
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
				EXPECT_TRUE(EqAbs(v1, v2, 8e-2f));
				EXPECT_TRUE(EqAbs(v1, v3, 8e-2f));
			}
		}
		TYPED_TEST(ExpQuaternionTest, Lerp) {
			using This_t = std::decay_t<decltype(*this)>;
			using EQ = typename This_t::ExpType;

			// ExpQuatを合成した結果をQuat合成のケースと比較
			for(int i=0 ; i<N_Iteration ; i++) {
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
				EXPECT_LE(err_sum, 0.4f);
			}
		}
		TYPED_TEST(QuaternionTest, ConvertMatrix) {
			using This_t = std::decay_t<decltype(*this)>;
			constexpr bool Align = This_t::Align;
			using QT = typename This_t::QuatType;
			auto& rd = this->refRand();
			auto rdf = rd.template getUniformF<float>();

			for(int i=0 ; i<N_Iteration ; i++) {
				RadF ang = RadF::Random(rdf);
				Vec3 axis = Vec3::RandomDir(rdf);
				QT q = QT::Rotation(axis, ang);
				auto m = AMat33::RotationAxis(axis, ang);

				// クォータニオンでベクトルを変換した結果が行列のそれと一致するか
				random::GenRVec<Vec3>(rdf, {-1e3f, 1e3f});
				auto v = VecT<3,Align>::Random(rdf, {-1e3f, 1e3f});
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
			auto& rd = this->refRand();
			auto rdf = rd.template getUniformF<float>();

			// クォータニオンを合成した結果を行列のケースと比較
			for(int i=0 ; i<N_Iteration ; i++) {
				RadF ang[2] = {RadF::Random(rdf), RadF::Random(rdf)};
				Vec3 axis[2] = {Vec3::RandomDir(rdf), Vec3::RandomDir(rdf)};
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
			auto& rd = this->refRand();
			auto rdf = rd.template getUniformF<float>();

			// getRight(), getUp(), getDir()が{1,0,0},{0,1,0},{0,0,1}を変換した結果と比較
			for(int i=0 ; i<N_Iteration ; i++) {
				RadF ang = RadF::Random(rdf);
				Vec3 axis = Vec3::RandomDir(rdf);
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
			auto& rd = this->refRand();

			// クォータニオンの線形補間
			for(int i=0 ; i<N_Iteration ; i++) {
				const int div = 32;
				float tdiv = 1.f/div;
				auto axis = this->genRandDir();
				RadF ang = RadF::Random(rd.template getUniformF<float>());
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
					EXPECT_TRUE(EqULPs(v0, v1, ThresholdULPs_Quat));
				}
			}
		}
		TEST_F(MathTest, DecompAffine) {
			constexpr RangeF range{-1e3f, 1e3f};
			auto rd = getRand();
			auto rdf = rd.template getUniformF<float>();

			// 行列をDecompAffineした結果を再度合成して同じかどうかチェック
			for(int i=0 ; i<N_Iteration ; i++) {
				auto q = AQuat::Random(rdf);
				auto t = Vec3::Random(rdf, range);
				auto s = Vec3::RandomWithLength(rdf, 1e-2f, range);

				Pose3D pose(t, q, s);
				auto m = pose.getToWorld();
				auto ap = AffineParts::Decomp(m);
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

		class YPDTest : public RandomTestInitializerP<int> {};
		// XZ -> YawAngle
		struct Yaw_Angle {
			Vec3	dir;
			DegF	angle;
		};
		const Yaw_Angle c_yawAngle[] = {
			{{0,0,1}, DegF(0)},
			{{1,0,1}, DegF(45)},
			{{1,0,0}, DegF(90)},
			{{1,0,-1}, DegF(135)},
			{{0,0,-1}, DegF(180)},
			{{-1,0,-1}, DegF(225)},
			{{-1,0,0}, DegF(270)},
			{{-1,0,1}, DegF(315)}
		};
		TEST_F(YPDTest, FromPos_Random) {
			constexpr RangeF range = {-1e3f, 1e3f};
			auto rd = getRand();
			auto rdf = rd.template getUniformF<float>();
			// YawPitchDistで計算した値を再度合成して結果を比較
			Vec3 nzv = Vec3::RandomWithLength(rdf, 1e-2f, range);
			auto ypd = YawPitchDist::FromPos(nzv);
			auto ofsRot = ypd.toOffsetRot();
			auto nzv2 = -ofsRot.second.getDir() * ypd.distance;

			constexpr auto ThULPs = ULPs_C(100.f, 0.3f);
			EXPECT_TRUE(EqULPs(ofsRot.first, nzv, ThULPs));
			// rotation + distanceから復元した元座標ベクトル
			EXPECT_TRUE(EqULPs(nzv2, nzv, ThULPs));
		}

		TEST_P(YPDTest, FromPos_Fixed) {
			auto rd = getRand();
			auto rdF = [&rd](){
				return rd.template getUniform<float>({-1.f, 1.f});
			};

			// 予め答えが用意されたパターンと照らし合わせ
			auto& ya = c_yawAngle[GetParam()];
			auto pos = ya.dir;
			pos.y = rdF();	// Pitchはランダムな値
			auto ypd = YawPitchDist::FromPos(pos);
			EXPECT_NEAR(DegF(ya.angle).get(), DegF(ypd.yaw).get(), 0.05f);
		}
		INSTANTIATE_TEST_CASE_P(FromPos_Fixed,
								YPDTest,
								::testing::Range(0, int(countof(c_yawAngle))));

		TEST_F(YPDTest, Direction) {
			// Quat::RotateYPRの結果と比較
			constexpr float RandMin = -1e2f,
							RandMax = 1e2f;
			auto rd = getRand();
			auto rdF = [RandMin, RandMax, &rd](){ return rd.template getUniform<float>({RandMin, RandMax}); };
			RadF yaw(rdF()),
				pitch(rdF());

			YawPitchDist ypd{yaw, pitch, 1.f};
			auto ofsRot = ypd.toOffsetRot();
			Vec3 v(0,0,1);
			auto q = AQuat::RotationYPR(yaw, pitch, RadF(0));
			v *= q;
			EXPECT_TRUE(EqAbs(ofsRot.first, v, 1e-5f));

			auto ori = ofsRot.first + ofsRot.second.getDir();
			EXPECT_TRUE(EqAbs(ori, Vec3(0,0,0), 1e-5f));
		}

		template <class T>
		class AngleTest : public RandomTestInitializer {
			protected:
				spn::Optional<MTRandom>	_rd;
				std::function<T ()>		_rdF;

				void SetUp() override {
					RandomTestInitializer::SetUp();
					_rd = getRand();
					constexpr T RandMin = T(-1e3),
								RandMax = T(1e3);
					_rdF = [this, RandMin, RandMax](){ return _rd->template getUniform<T>({RandMin, RandMax}); };
				}
				void TearDown() override {
					_rd = none;
					RandomTestInitializer::TearDown();
				}
			public:
		};
		using AngleTypeList = ::testing::Types<float, double>;
		TYPED_TEST_CASE(AngleTest, AngleTypeList);

		TYPED_TEST(AngleTest, Loop) {
			constexpr int N_Iterations = 100;
			constexpr auto ThULPs = ULPs_C(TypeParam(1.0), TypeParam(0.001));
			constexpr auto onerotation = AngleInfo<Degree_t>::onerotation<TypeParam>;
			// singleテスト
			// ループ毎にsingleした値と独自にwhileで求めた値を比べる
			auto angle = this->_rdF();
			Degree<TypeParam> degf(angle);
			for(int i=0 ; i<N_Iterations ; i++) {
				auto tmp_degf = degf;
				{
					tmp_degf.single();

					auto val = angle;
					while(val >= onerotation)
						val -= onerotation;
					while(val < 0)
						val += onerotation;
					EXPECT_TRUE(EqULPs(tmp_degf.get(), val, ThULPs));
				}

				auto a = this->_rdF();
				angle += a;
				degf.set(degf.get() + a);
			}
		}
		TYPED_TEST(AngleTest, Range) {
			// rangeテスト
			Degree<TypeParam> degf;
			auto fnRange = [this]() {
				auto rmin = this->_rdF(),
					 rmax = this->_rdF();
				if(rmin > rmax)
					std::swap(rmin, rmax);
				return std::make_pair(rmin, rmax);
			};
			auto r = fnRange();
			auto val = Saturate(degf.get(), r.first, r.second);
			degf.range(Degree<TypeParam>(r.first), Degree<TypeParam>(r.second));
			EXPECT_EQ(degf.get(), val);

			// rangeValueテスト
			degf.set(this->_rdF());
			r = fnRange();
			val = Saturate(degf.get(), r.first, r.second);
			degf.rangeValue(r.first, r.second);
			EXPECT_EQ(degf.get(), val);
		}
		TYPED_TEST(AngleTest, Degree_Radian_Degree) {
			constexpr auto ThULPs = ULPs_C(TypeParam(1000.0), TypeParam(0.1));
			// Degree -> Radian -> Degree で値の比較
			Degree<TypeParam> degf(this->_rdF());
			auto radf = degf.template convert<Radian_t>();
			auto degf2 = radf.template convert<Degree_t>();
			EXPECT_TRUE(EqULPs(degf.get(), degf2.get(), ThULPs));
		}
		TYPED_TEST(AngleTest, Degree_Radian_Convert) {
			auto deg = this->_rdF();
			Degree<TypeParam> degf(deg);
			Radian<TypeParam> radf(degf);
			auto r0 = degf.get() / AngleInfo<Degree_t>::onerotation<TypeParam>;
			auto r1 = radf.get() / AngleInfo<Radian_t>::onerotation<TypeParam>;
			EXPECT_NEAR(r0, r1, TypeParam(1e-4));
		}
		TYPED_TEST(AngleTest, Arithmetic) {
			Degree<TypeParam> degf(this->_rdF()),
								degf2(this->_rdF());
			EXPECT_EQ((degf + degf2).get(), degf.get() + degf2.get());
			EXPECT_EQ((degf - degf2).get(), degf.get() - degf2.get());
			EXPECT_EQ((degf * 2).get(), degf.get() * 2);
			EXPECT_EQ((degf / 2).get(), degf.get() / 2);

			auto val = static_cast<TypeParam>(degf) + static_cast<TypeParam>(degf2);
			EXPECT_EQ((degf += degf2).get(), val);
			val = static_cast<TypeParam>(degf) - static_cast<TypeParam>(degf2);
			EXPECT_EQ((degf -= degf2).get(), val);
			val = static_cast<TypeParam>(degf) * 2;
			EXPECT_EQ((degf *= 2).get(), val);
			val = static_cast<TypeParam>(degf) / 2;
			EXPECT_EQ((degf /= 2).get(), val);
		}
		TYPED_TEST(AngleTest, Lerp) {
			Degree<TypeParam> ang0(this->_rdF()),
								ang1(this->_rdF()),
								tmp, tmp2;
			ang0.single();
			ang1.single();
			// Lerp係数が0ならang0と等しい
			tmp = AngleLerp(ang0, ang1, 0.0);
			tmp.single();
			EXPECT_NEAR(ang0.get(), tmp.get(), 1e-4);
			// Lerp係数が1ならang1と等しい
			tmp = AngleLerp(ang0, ang1, 1.0);
			tmp.single();
			EXPECT_NEAR(ang1.get(), tmp.get(), 1e-4);
			// Lerp係数を0.5で2回かければ0.75でやったのと(ほぼ)同じになる
			tmp = AngleLerp(ang0, ang1, .5);
			tmp = AngleLerp(tmp, ang1, .5);
			tmp2 = AngleLerp(ang0, ang1, .75);
			tmp.single(); tmp2.single();
			EXPECT_NEAR(tmp.get(), tmp2.get(), 1e-4);
		}
		TYPED_TEST(AngleTest, Move) {
			using Ang = Degree<TypeParam>;
			Ang		ang0(this->_rdF()),
					ang1(this->_rdF()),
					tmp;
			auto fnDiff = [&tmp, &ang1](){
				return std::abs(AngleLerpValueDiff(tmp.get(), ang1.get(), Ang::OneRotationAng));
			};
			ang0.single();
			ang1.single();
			tmp = ang0;
			auto diff = AngleLerpValueDiff(ang0.get(), ang1.get(), Ang::OneRotationAng);
			// 角度の差分を3で割ったら3回処理した時点でang1とほぼ等しくなる
			auto diff3 = Ang(std::abs(diff / 3.f));
			tmp = AngleMove(tmp, ang1, diff3);
			EXPECT_FALSE(fnDiff() < 1e-4f);
			tmp = AngleMove(tmp, ang1, diff3);
			EXPECT_FALSE(fnDiff() < 1e-4f);
			tmp = AngleMove(tmp, ang1, diff3);
			EXPECT_TRUE(fnDiff() < 1e-4f);
		}
		TYPED_TEST(AngleTest, AngleValue) {
			using Ang = Degree<TypeParam>;
			Ang	ang(this->_rdF());
			// VectorFromAngleで計算したものと行列で計算したものとはほぼ一致する
			auto v0 = VectorFromAngle(ang),
				 v1 = Vec2(0,1) * Mat22::Rotation(ang);
			EXPECT_NEAR(v0.x, v1.x, 1e-4f);
			EXPECT_NEAR(v0.y, v1.y, 1e-4f);

			// AngleValueにかければ元の角度が出る
			Ang ang1 = AngleValue(v0);
			ang.single();
			ang1.single();
			EXPECT_NEAR(ang1.get(), ang.get(), 1e-2f);
		}
		namespace {
			template <class T>
			void TestAsIntegral(MTRandom rd) {
				auto fvalue = rd.getUniform<T>({std::numeric_limits<T>::lowest()/2,
														std::numeric_limits<T>::max()/2});
				auto ivalueC = AsIntegral_C(fvalue);
				auto ivalue = AsIntegral(fvalue);
				EXPECT_EQ(ivalueC, ivalue);
			}
			template <class T>
			void TestCompare(MTRandom rd) {
				auto fnRand = [&rd]() {
					auto range0 = static_cast<T>(std::pow(T(2), std::numeric_limits<T>::max_exponent/2));
					return rd.getUniform<T>({-range0, range0});
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
		TEST_F(MathTest, BarycentricCoord) {
			auto rd = getRand();
			auto rv = [&](){ return Vec2::Random(rd.getUniformF<float>(), {-5e1f,5e1f}); };

			Vec2 vtx[] = {rv(), rv(), rv()},
				pos = rv(),
				range;
			float dt = std::abs((vtx[2]-vtx[0]).cw(vtx[1]-vtx[0]));
			float c[3];
			dt = Rcp22Bit(std::max(1e-8f, dt));
			auto threshold = (ULPs_Increment(dt) - dt) * (1<<24);
			// 重心座標を求め、それを使って合成した座標が元と一致するか
			BarycentricCoord(c, vtx[0], vtx[1], vtx[2], pos);
			float dist = pos.distance(MixVector(c, vtx[0], vtx[1], vtx[2]));
			EXPECT_TRUE(EqAbs(dist, 0.f,  threshold));
		}
		TEST_F(MathTest, GapStructure) {
			auto rd = getRand();
			float (MTRandom::*FPtr)() = &MTRandom::getUniform<float>;
			auto gen = std::bind(FPtr, std::ref(rd));
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
