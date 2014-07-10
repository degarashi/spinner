#ifdef WIN32
	#include <intrin.h>
#endif
#include "math.hpp"
#include "../vector.hpp"
#include "../matrix.hpp"
#include "../matstack.hpp"
#include "../pose.hpp"
#include "../bits.hpp"
#include "../noseq.hpp"
#include "test.hpp"
#include "../random.hpp"

namespace spn {
	namespace test {
		#define MATRIXCHECK(z, dummy, dim)	MatrixCheck<BOOST_PP_TUPLE_ELEM(0,dim), BOOST_PP_TUPLE_ELEM(1,dim)>();
		namespace {
			constexpr float Threshold = 5e-4f,
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
			template <class RD, class AR, int M, int N, bool A>
			void SetRandom(RD& rd, AR& ar, MatT<M,N,A>& m) {
				for(int i=0 ; i<M ; i++) {
					for(int j=0 ; j<N ; j++) {
						m.ma[i][j] = ar[i][j] = rd();
					}
				}
			}
			template <class AR, int M, int N, bool A>
			bool Compare(const AR& ar, const MatT<M,N,A>& m) {
				for(int i=0 ; i<M ; i++) {
					for(int j=0 ; j<N ; j++) {
						auto diff = ar[i][j] - m.ma[i][j];
						if(std::fabs(diff) >= Threshold)
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
						Assert(Trap, Compare(ar0, m0))
					}
				};
				// + mat
				fnTestMat([](auto& a0, auto& a1, auto&&){ Operate1(a0,a1,OpAdd); }, OpAdd);
				// - mat
				fnTestMat([](auto& a0, auto& a1, auto&&){ Operate1(a0,a1,OpSub); }, OpSub);
				// * mat
				auto* ptr = &MulArray<std::array<std::array<float, N>, N>, decltype(OpMul)>;
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
				Assert(Trap, Compare(ar0, m0))
			}
			template <int M, int N, bool A>
			void _MatrixCheck() {
				// 配列を使って計算した結果とMatrixクラスの結果を、誤差含めて比較
				auto rd = mgr_random.get(0);
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
						Assert(Trap, Compare(ar0, m0))
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
			void MatrixCheck() {
				_MatrixCheck<M,N,false>();
				_MatrixCheck<M,N,true>();
			}
		}
		void MatTest() {
			BOOST_PP_SEQ_FOR_EACH(MATRIXCHECK, BOOST_PP_IDENTITY, SEQ_MATDEF)
		}
		void PoseTest() {
			Pose2D p2(AVec2(100,200), 128, AVec2(1,1)),
			p3(AVec2(1,2), 64, AVec2(3,3));
			auto p4 = p2.lerp(p3, 0.5f);
			p4 = p4;

			auto pc = AVec3::FromPacked(MakeChunk(64,128,192,255));
			auto pc2 = pc.toPacked();

			noseq_list<float> ls;
			auto id0 = ls.add(256);
			auto id1 = ls.add(512);
			auto id2 = ls.add(1024);
			std::cout << std::hex
					<< id0 << std::endl
					<< id1 << std::endl
					<< id2 << std::endl;
			ls.rem(id1);
			auto re = ls.alloc();
			re.second = 1.234f;
			for(auto& p : ls) {
				std::cout << p << std::endl;
			}
		}
		void BitFieldTest() {
			struct MyDef : BitDef<uint32_t, BitF<0,14>, BitF<14,6>, BitF<20,12>> {
				enum { VAL0, VAL1, VAL2 }; };
			using Value = BitField<MyDef>;
			Value value(0);
			value.at<Value::VAL2>() = ~0;
			auto mask = value.mask<Value::VAL1>();
			auto raw = value.value();
			std::cout << std::hex << "raw" << raw
									<< "mask" << mask << std::endl;
		}
		void GapTest() {
			auto rd = mgr_random.get(0);
			auto gen = std::bind(&MTRandom::getUniform<float>, std::ref(rd));
			using F16 = std::array<float,16>;
			F16 tmp;
			std::generate(tmp.begin(), tmp.end(), std::ref(gen));

			#define SETAUX(z,n,data)	aux##n = *data++;
			#define CHECKAUX(z,n,data) Assert(Trap, aux##n==*data++)
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
