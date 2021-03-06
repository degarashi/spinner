//! 行列クラス基底ヘッダ
/*! 他行列との演算を除いたメソッドを定義 */
#if !BOOST_PP_IS_ITERATING
	#if !defined(MATRIX_H_) || INCLUDE_MATRIX_LEVEL >= 1
		#define MATRIX_H_
		#include <boost/preprocessor.hpp>
		#include <boost/serialization/access.hpp>
		#include <boost/serialization/level.hpp>
		#include <cstring>
		#include <stdexcept>
		#include <cassert>
		#include "vector.hpp"
		#include "tostring.hpp"
		#include "structure/angle.hpp"
		#include "random/matrix.hpp"

		// 定義する行列の次元(M,N)
		#define SEQ_MATDEF	((2,2))((2,3))((2,4))((4,2))((3,2))((3,3))((3,4))((4,3))((4,4))
		#define LEN_SEQ		BOOST_PP_SEQ_SIZE(SEQ_MATDEF)
		#define LEN_SEQ_M1	BOOST_PP_DEC(LEN_SEQ)

		// 要求された定義レベルを実体化
		#ifndef INCLUDE_MATRIX_LEVEL
			#define INCLUDE_MATRIX_LEVEL 0
		#endif
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LEN_SEQ_M1, "matrix.hpp", INCLUDE_MATRIX_LEVEL))
		#include BOOST_PP_ITERATE()
		#undef INCLUDE_MATRIX_LEVEL
	#endif
#elif BOOST_PP_ITERATION_DEPTH() == 1
	// アラインメントイテレーション
	#define BOOST_PP_ITERATION_PARAMS_2 (3, (0, 1, "matrix.hpp"))
	#include BOOST_PP_ITERATE()
#else
	#define TUP	BOOST_PP_SEQ_ELEM(BOOST_PP_FRAME_ITERATION(1), SEQ_MATDEF)
	#define DIM_M	BOOST_PP_TUPLE_ELEM(0, TUP)
	#define DIM_N	BOOST_PP_TUPLE_ELEM(1, TUP)
	#define ALIGN	BOOST_PP_ITERATION()
	#define ALIGNB	BOOLNIZE(ALIGN)
	#define ALIGN16 BOOST_PP_IF(ALIGN, alignas(16), NOTHING)
	#define MT		MatT<DIM_M, DIM_N, ALIGNB>
	#define DMAX 	BOOST_PP_MAX(DIM_M, DIM_N)
	#define DMIN 	BOOST_PP_MIN(DIM_M, DIM_N)
	#define DMUL	BOOST_PP_MUL(DIM_M, DIM_N)

	#define DIMTUPLE(n,i)				BOOST_PP_TUPLE_ELEM(i, BOOST_PP_SEQ_ELEM(n, SEQ_MATDEF))
	#define DEF_CONV_ITR(z,n,data)		data(DIMTUPLE(n,0), DIMTUPLE(n,1), 0) data(DIMTUPLE(n,0), DIMTUPLE(n,1), 1)

	#define DIM		DIM_N
	#define DEFINE_MATRIX
	#include "local_macro.hpp"

	#if BOOST_PP_FRAME_FLAGS(1) == 0
		namespace spn {
			// row-major
			template <>
			struct ALIGN16 MatT<DIM_M, DIM_N, ALIGNB> : MatBase, boost::equality_comparable<MT> {
				using Row = VecT<DIM_N, ALIGNB>;			//!< 行を表すベクトル型
				using Column = VecT<DIM_M, false>;			//!< 列を表すベクトル型
				using RowE = VecT<4, ALIGNB>;				//!< 4要素の行ベクトル型
				using AMat = MatT<DIM_M, DIM_N, true>;
				using UMat = MatT<DIM_M, DIM_N, false>;
				using UVec2 = VecT<2,false>;
				using UVec3 = VecT<3,false>;

				constexpr static int width = DIM_N,
									height = DIM_M,
								PaddingedSize = BOOST_PP_IF(ALIGN, 4, DIM_N),
								AlignedSize = DIM_M*4-(4-DIM_N),
								UnAlignedSize = DIM_M*DIM_N;
				constexpr static bool align = ALIGNB;
				union {
					//! 容量確保用
					float	data[BOOST_PP_IF(ALIGN, AlignedSize, UnAlignedSize)];
					//! 2次元配列アクセス用
					float	ma[0][PaddingedSize];
				};

				friend class boost::serialization::access;
				template <class Archive>
				void serialize(Archive& ar, const unsigned int /*ver*/) {
					ar & BOOST_SERIALIZATION_NVP(data);
				}

				// -------------------- ctor --------------------
				MatT() = default;
				MatT(const AMat& m);
				MatT(const UMat& m);
				MatT(std::initializer_list<float> il);
				MatT& mul_self();

				#define DIAGONAL2(z,n1,n0)		ma[n0][n1] = BOOST_PP_IF(BOOST_PP_EQUAL(n0,n1), s, 0);
				#define DIAGONAL(z,n,data)		BOOST_PP_REPEAT_##z(DIM_N, DIAGONAL2, n)
				MatT(float s, _TagDiagonal) {
					BOOST_PP_REPEAT(DIM_M, DIAGONAL, NOTHING)
				}
				#define ALLSET(z,n,data)	STORETHIS(n, data);
				MatT(float s, _TagAll) {
					reg128 r = reg_load1_ps(&s);
					BOOST_PP_REPEAT(DIM_M, ALLSET, r)
				}
				MatT(_TagIdentity): MatT(1.f, TagDiagonal) {}

				#define SETARRAY(z,n,src)	STORETHIS(n, reg_loadu_ps(src)); src += DIM_N;
				MatT(const float* src) {
					BOOST_PP_REPEAT(DIM_M, SETARRAY, src)
				}
				template <int N>
				float& ref1D() {
					return ma[N/DIM_N][N%DIM_N];
				}

				#define DEF_ARGS(z,n,data)	(float BOOST_PP_CAT(data,n))
				#define SET_ARGS0(z,n,data)	ref1D<n>() = BOOST_PP_CAT(data,n);
				//! 全てを指定
				MatT(BOOST_PP_SEQ_ENUM(BOOST_PP_REPEAT(DMUL, DEF_ARGS, f))) {
					BOOST_PP_REPEAT(DMUL, SET_ARGS0, f)
				}
				#define SET_ARGS1(z,n,data)	STORETHIS(n, reg_mul_ps(xmm_const::matI()[n], reg_load1_ps(&BOOST_PP_CAT(data,n))));
				#define SET_ARGS2(z,n,dummy) STORETHIS(n, reg_setzero_ps());
				//! 対角線上
				MatT(BOOST_PP_SEQ_ENUM(BOOST_PP_REPEAT(DMIN, DEF_ARGS, f))) {
					BOOST_PP_REPEAT(DMIN, SET_ARGS1, f)
					BOOST_PP_REPEAT_FROM_TO(DMIN, DIM_M, SET_ARGS2, NOTHING)
				}
				#undef SET_ARGS2
				template <bool A>
				bool operator == (const MatT<DIM_M, DIM_N, A>& m) const;

				// -------------------- query values --------------------
				#if ALIGN==1 && BOOST_PP_LESS(DIM_N,4)==1
					template <class T>
					T& getAuxRef(int m, int n) {
						return *reinterpret_cast<T*>(ma[m]+DIM_N+n);
					}
					template <class T>
					const T& getAuxRef(int m, int n) const {
						return *reinterpret_cast<const T*>(ma[m]+DIM_N+n);
					}
				#endif

				const Row& getRow(int n) const {
					if(n >= height)
						return *reinterpret_cast<const Row*>(xmm_const::cs_matI()[n]);
					return *reinterpret_cast<const Row*>(ma[n]);
				}
				Row& getRow(int n) {
					return *reinterpret_cast<Row*>(ma[n]);
				}
				//! 行を取得(最大4まで)
				/*! 足りない分は対角要素のみ1それ以外は0とみなして取得 */
				#if ALIGN==0 || DIM_N==4
					// 普通のgetRowと同義
					const Row& getRowE(int n) const {
						return getRow(n); }
				#else
					RowE getRowE(int n) const {
						if(n >= height)
							return RowE(xmm_const::matI()[n]);
						return RowE(reg_or_ps(reg_and_ps(LOADTHIS(n), xmm_const::mask()[n]), xmm_const::matI()[n]));
					}
				#endif
				template <bool A>
				void setRow(int n, const VecT<DIM_N,A>& v) {
					getRow(n) = v.loadPS();
				}
				void setColumn(int n, const Column& v) {
					for(int i=0 ; i<DIM_M ; i++)
						ma[i][n] = v.m[i];
				}
				#define SET_COLUMN(z,n,data)	(ma[n][data])
				Column getColumn(int n) const {
					if(n >= DIM_N)
						return Column(xmm_const::matI()[n]);
					return Column(BOOST_PP_SEQ_ENUM(BOOST_PP_REPEAT(DIM_M, SET_COLUMN, n)));
				}
				#undef SET_COLUMN

				#define DEF_GETROW(z,n,data)	BOOST_PP_CAT(const Row& getRow, n)() const { return *reinterpret_cast<const Row*>(ma[n]); }
				//! マクロからのアクセス用: 行毎に個別のメンバ関数を用意
				/*! マクロのColumn取得は多分、不要 */
				BOOST_PP_REPEAT(DIM_M, DEF_GETROW, NOTHING)
				#undef DEF_GETROW

				// -------------------- operators --------------------
				#define FUNC(z,n,func)		STORETHIS(n, func(LOADTHIS(n), r0));
				#define FUNC2(z,n,func)		STORETHISPS(ret.ma[n], func(LOADTHIS(n), r0));
				#define DEF_OP(op, func) MatT& operator BOOST_PP_CAT(op,=) (float s) { \
					reg128 r0 = reg_load1_ps(&s); \
					BOOST_PP_REPEAT(DIM_M, FUNC, func) \
					return *this; } \
					MatT operator op (float s) const { \
						MatT ret; \
						reg128 r0 = reg_load1_ps(&s); \
						BOOST_PP_REPEAT(DIM_M, FUNC2, func) \
						return ret; } \
					friend MatT operator op (float s, const MatT& m);

				DEF_OP(+, reg_add_ps)
				DEF_OP(-, reg_sub_ps)
				DEF_OP(*, reg_mul_ps)
				DEF_OP(/, _mmDivPs)
				#undef DEF_OP
				#undef FUNC
				#undef FUNC2

				#define FUNC3(z,n,func)		STORETHIS(n, func(LOADTHIS(n), LOADTHISPS(m.ma[n])));
				#define FUNC4(z,n,func)		STORETHISPS(ret.ma[n], func(LOADTHIS(n), LOADTHISPS(m.ma[n])));
				#define DEF_OPM(op, func) MatT& operator BOOST_PP_CAT(op,=) (const MatT& m) { \
					BOOST_PP_REPEAT(DIM_M, FUNC3, func) \
					return *this; } \
				MatT operator op (const MatT& m) const { \
					MatT ret; \
					BOOST_PP_REPEAT(DIM_M, FUNC4, func) \
					return ret; }

				DEF_OPM(+, reg_add_ps)
				DEF_OPM(-, reg_sub_ps)
				#undef FUNC3
				#undef FUNC4

				// -------------------- others --------------------
				// 行列拡張Get = Mat::getRowE()
				void identity();
				static MatT Scaling(BOOST_PP_SEQ_ENUM(BOOST_PP_REPEAT(DMIN, DEF_ARGS, f)));
				#if (DMIN == 2 || DMIN == 3) && DIM_M == 3
					//! 2次元移動
					static MatT Translation(const UVec2& v);
				#endif
				#if DIM_M == 4 && DIM_N >= 3
					//! 3次元移動
					static MatT Translation(const UVec3& v);
					static MatT LookAtLH(const UVec3& pos, const UVec3& at, const UVec3& up);
					static MatT LookDirLH(const UVec3& pos, const UVec3& dir, const UVec3& up);
					static MatT LookAtRH(const UVec3& pos, const UVec3& at, const UVec3& up);
					static MatT LookDirRH(const UVec3& pos, const UVec3& dir, const UVec3& up);
				#endif
				#if DMIN == 2
					//! 2D回転
					static MatT Rotation(RadF ang);
				#elif DMIN >= 3
						//! X軸周りの回転
						static MatT RotationX(RadF ang);
						//! Y軸周りの回転
						static MatT RotationY(RadF ang);
						//! Z軸周りの回転
						static MatT RotationZ(RadF ang);
						//! 任意軸周りの回転
						static MatT RotationAxis(const UVec3& axis, RadF ang);
					#if DMIN == 4
						//! 透視変換行列
						static MatT PerspectiveFovLH(RadF fov, float aspect, float nz, float fz);
						static MatT PerspectiveFovRH(RadF fov, float aspect, float nz, float fz);
						static MatT _PerspectiveFov(RadF fov, float aspect, float nz, float fz, float coeff);
					#endif
				#endif

				// 正方行列限定のルーチン
				#if DIM_M==DIM_N
					void transpose();
					//! 固有値計算
					float calcDeterminant() const;
					// TODO: 固有ベクトル計算
					//! 逆行列計算
					bool inversion(MatT& dst) const;
					bool inversion(MatT& dst, float det) const;
					bool invert();
					// -------- Luaへのエクスポート用 --------
					MatT addF(float s) const;
					MatT addM(const MatT& m) const;
					MatT subF(float s) const;
					MatT subM(const MatT& m) const;
					VecT<DIM_N,align> mulV(const VecT<DIM_N,align>& v) const;
					MatT mulF(float s) const;
					MatT mulM(const MatT& m) const;
					MatT divF(float s) const;
					MatT luaInvert() const;
				#endif
				std::string toString() const;
				MatT<DIM_N,DIM_M,ALIGNB> transposition() const;

				#if DMIN > 2
					//! 指定した行と列を省いた物を出力
					MatT<height-1,width-1,ALIGNB> cutRC(int row, int clm) const;
				#endif
				// ---------- < 行の基本操作 > ----------
				//! 行を入れ替える
				void rowSwap(int r0, int r1);
				//! ある行を定数倍する
				void rowMul(int r0, float s);
				//! ある行を定数倍した物を別の行へ足す
				void rowMulAdd(int r0, float s, int r1);
				// ---------- < 列の基本操作 > ----------
				//! 列を入れ替える
				void clmSwap(int c0, int c1);
				//! ある列を定数倍する
				void clmMul(int c0, float s);
				//! ある列を定数倍した物を別の行へ足す
				void clmMulAdd(int c0, float s, int c1);

				//! ある行の要素が全てゼロか判定 (誤差=EPSILON込み)
				bool isZeroRowEps(int n) const;
				bool isZeroRow(int n) const;
				//! 零行列か判定
				bool isZero() const;
				//! 各行を正規化する (最大の係数が1になるように)
				void rowNormalize();
				//! 被約形かどうか判定
				bool isEchelon() const;
				//! 被約形にする
				/*! \return 0の行数 */
				int rowReduce();

				template <class RDF>
				static MatT Random(const RDF& rdf, const RangeF& r=random::DefaultRMatRange) {
					return random::GenRMat<MatT>(rdf, r);
				}

				// TODO: DecompAffine [DIM >= 3]

				// 本来行列の積算が出来るのはDIM_N == 相手のDIM_Mの時だけだが
				// 足りない分をgetRowE()で補う事で可能にする
				// 演算結果も本当は(DIM_M, Other::DIM_N)となるが
				// convert<Tgt::DIM_M, Tgt::DIM_N>(result) とする事で合わせる
				// max(DIM_N, Other::DIM_M)
				// 演算子の出力自体はサイズそのまま
				// Mat34 a;
				// Mat44 b;
				// a * b -> 3x4
				// a * [3x4](4x4) -> 3x4
				// TODO: 入力が出力と同じサイズで、なおかつrvalueだったら&&で受け取ってそのアドレスを返す

				//! 行列サイズを変更
				/*! 縮小の場合は要素を削り
					拡大の際に足りない成分は対角=1, 他は0とする */
				// デバッグ時速度を考えるとテンプレートで実装できない(？)のでマクロ使用
				// SEQ_MATDEFの組み合わせを生成 + (Aligned | UnAligned)
				#define CONVFUNC(n0,n1,align)	BOOST_PP_CAT(BOOST_PP_CAT(BOOST_PP_CAT(convert,AFLAG(align)), n0),n1)
				#define DEF_CONV(n0,n1,align)	MatT<n0,n1,BOOLNIZE(align)> CONVFUNC(n0,n1,align)() const; \
												void convert(MatT<n0,n1,BOOLNIZE(align)>& dst) const;
				BOOST_PP_REPEAT(LEN_SEQ, DEF_CONV_ITR, DEF_CONV)
				#undef DEF_CONV
				#undef CONVFUNC
				// サイズの合わない行列(DIM_N != n0)の演算関数は定義しない
				//! 行列との積算 (3 operands)
				#define DEF_MUL0(n0,n1,align) \
					BOOST_PP_IF( \
						BOOST_PP_EQUAL(DIM_N, n0), \
						DEF_MUL, \
						NOTHING_ARG \
					)(n0,n1,align)
				#define DEF_MUL(n0,n1,align)	MatT<DIM_M, n1, ALIGNB> operator * (const MatT<n0,n1,BOOLNIZE(align)>& m) const;
				BOOST_PP_REPEAT(LEN_SEQ, DEF_CONV_ITR, DEF_MUL0)
				#undef DEF_MUL
				#undef DEF_MUL0
				//! 行列との積算 (2 operands)
				#define DEF_MULE0(n0,n1,align) \
					BOOST_PP_IF( \
						BOOST_PP_AND( \
							BOOST_PP_EQUAL(DIM_N, n0), \
							BOOST_PP_EQUAL(n1, DIM_N) \
						), \
						DEF_MULE, \
						NOTHING_ARG \
					)(n0,n1,align)
				#define DEF_MULE(n0,n1,align)	MatT& operator *= (const MatT<n0,n1,BOOLNIZE(align)>& m);
				BOOST_PP_REPEAT(LEN_SEQ, DEF_CONV_ITR, DEF_MULE0)
				#undef DEF_MULE
				#undef DEF_MULE0
				//! 行列の代入
				MT& operator = (const MT& m);
				MT& operator = (std::initializer_list<float> il);

				//! 行列との積算 (右から掛ける)
				/*! 列ベクトルとして扱う = ベクトルを転置して左から行ベクトルを掛ける */
				template <bool A>
				VecT<DIM_N,ALIGNB> operator * (const VecT<DIM_N,A>& v) const;

				friend std::ostream& operator << (std::ostream& os, const MT& m);
			};
			// 使いやすいように別名を定義 ex. MatT<2,2,true> = AMat22
			using BOOST_PP_CAT(
					BOOST_PP_CAT(
						BOOST_PP_CAT(AFLAG(ALIGN),Mat)
						,DIM_M)
					,DIM_N)
					= MatT<DIM_M,DIM_N,ALIGNB>;
		}
	BOOST_CLASS_IMPLEMENTATION(spn::MT, object_serializable)
	#elif BOOST_PP_FRAME_FLAGS(1) == 1
		namespace spn {
			#define FUNC_OP(z,n,func)	STORETHISPS(ret.ma[n], func(r, LOADTHISPS(m.ma[n])));
			#define DEF_OPERATOR_FUNC(op, func) MT operator op (float s, const MT& m) { \
				MT ret; \
				reg128 r = reg_load1_ps(&s); \
				BOOST_PP_REPEAT(DIM_M, FUNC_OP, func) \
				return ret; \
			}
			DEF_OPERATOR_FUNC(+, reg_add_ps)
			DEF_OPERATOR_FUNC(-, reg_sub_ps)
			DEF_OPERATOR_FUNC(*, reg_mul_ps)
			DEF_OPERATOR_FUNC(/, _mmDivPs)
			#undef FUNC_OP
			#undef DEF_OPERATOR_FUNC

			MT::MatT(const AMat& m) {
				for(int i=0 ; i<DIM_M ; i++)
					STORETHIS(i, LOADPS(m.ma[i]));
			}
			MT::MatT(const UMat& m) {
				for(int i=0 ; i<DIM_M ; i++)
					STORETHIS(i, LOADPSU(m.ma[i]));
			}
			MT::MatT(std::initializer_list<float> il) {
				// 足りない要素分はすべてゼロにする
				alignas(16) float tmp[DIM_M*DIM_N] = {};
				auto* pTmp = tmp;
				for(auto itr=il.begin() ; itr!=il.end() ; ++itr)
					*pTmp++ = *itr;
				for(int i=0 ; i<DIM_M ; i++)
					STORETHIS(i, LOADPSU(tmp+i*DIM_N));
			}
			// 行列拡張Get = Mat::getRowE()
			void MT::identity() {
				*this = MatT(1.0f, TagDiagonal);
			}
			std::string MT::toString() const {
				return ToString(*this);
			}
			std::ostream& operator << (std::ostream& os, const MT& m) {
				os << BOOST_PP_IF(ALIGN, 'A', ' ') << "Mat" << DIM_M << DIM_N << '[';
				for(int i=0 ; i<DIM_M ; i++) {
					for(int j=0 ; j<DIM_N-1 ; j++)
						os << m.ma[i][j] << ',';
					os << m.ma[i][DIM_N-1] << ' ';
				}
				return os << ']';
			}
			template <bool A>
			bool MT::operator == (const MatT<DIM_M, DIM_N, A>& m) const {
				for(int i=0 ; i<DIM_M ; i++) {
					if(getRow(i) != Row(m.getRow(i)))
						return false;
				}
				return true;
			}
			template bool MT::operator == (const MatT<DIM_M, DIM_N, false>&) const;
			template bool MT::operator == (const MatT<DIM_M, DIM_N, true>&) const;

			#define DEF_COPY(z,n,dummy)	STORETHISPS(ma[n], LOADTHISPS(m.ma[n]));
			MT& MT::operator = (const MT& m) {
				BOOST_PP_REPEAT(DIM_M, DEF_COPY, NOTHING)
				return *this;
			}
			#undef DEF_COPY
			MT& MT::operator = (std::initializer_list<float> il) {
				return *this = MT(il);
			}
			#define SET_ARGS2(z,n,data) (BOOST_PP_CAT(data, n))
			MT MT::Scaling(BOOST_PP_SEQ_ENUM(BOOST_PP_REPEAT(DMIN, DEF_ARGS, f))) {
				return MatT(BOOST_PP_SEQ_ENUM(BOOST_PP_REPEAT(DMIN, SET_ARGS2, f)));
			}
			#undef SET_ARGS2
			#if (DMIN == 2 || DMIN == 3) && DIM_M == 3
				MT MT::Translation(const VecT<2,false>& v) {
					MatT mt(TagIdentity);
					mt.ma[2][0] = v.x;
					mt.ma[2][1] = v.y;
					return mt;
				}
			#endif
			#if DIM_M == 4 && DIM_N >= 3
				MT MT::Translation(const UVec3& v) {
					MatT mt(TagIdentity);
					mt.ma[3][0] = v.x;
					mt.ma[3][1] = v.y;
					mt.ma[3][2] = v.z;
					return mt;
				}
				MT MT::LookAtLH(const UVec3& pos, const UVec3& at, const UVec3& up) {
					return LookDirLH(pos, (at-pos).normalization(), up);
				}
				MT MT::LookDirLH(const UVec3& pos, const UVec3& dir, const UVec3& up) {
					AVec3 xA(up % dir);
					xA.normalize();

					MatT ret;
					STORETHISPS(ret.ma[0], reg_setr_ps(xA.x, up.x, dir.x, 0));
					STORETHISPS(ret.ma[1], reg_setr_ps(xA.y, up.y, dir.y, 0));
					STORETHISPS(ret.ma[2], reg_setr_ps(xA.z, up.z, dir.z, 0));
					STORETHISPS(ret.ma[3], reg_setr_ps(-pos.dot(xA), -pos.dot(up), -pos.dot(dir), 1));
					return ret;
				}
				MT MT::LookAtRH(const UVec3& pos, const UVec3& at, const UVec3& up) {
					return LookDirLH(pos, (pos-at).normalization(), up);
				}
				MT MT::LookDirRH(const UVec3& pos, const UVec3& dir, const UVec3& up) {
					return LookDirLH(pos, -dir, up);
				}
			#endif
			#if DMIN == 2
				MT MT::Rotation(RadF ang) {
					const float S = std::sin(ang.get()),
								C = std::cos(ang.get());
					MatT mt(TagIdentity);
					mt.ma[0][0] = C;
					mt.ma[0][1] = S;
					mt.ma[1][0] = -S;
					mt.ma[1][1] = C;
					return mt;
				}
			#elif DMIN >= 3
					MT MT::RotationX(RadF ang) {
						const float C = std::cos(ang.get()),
									S = std::sin(ang.get());
						MatT mt;
						STORETHISPS(mt.ma[0], reg_setr_ps(1,	0,	0,	0));
						STORETHISPS(mt.ma[1], reg_setr_ps(0,	C,	S,	0));
						STORETHISPS(mt.ma[2], reg_setr_ps(0,	-S,	C,	0));
						#if DIM_M == 4
							STORETHISPS(mt.ma[3], reg_setr_ps(0,0,0,1));
						#endif
						return mt;
					}
					MT MT::RotationY(RadF ang) {
						const float C = std::cos(ang.get()),
									S = std::sin(ang.get());
						MatT mt;
						STORETHISPS(mt.ma[0], reg_setr_ps(C,	0,	-S,	0));
						STORETHISPS(mt.ma[1], reg_setr_ps(0,	1,	0,	0));
						STORETHISPS(mt.ma[2], reg_setr_ps(S,	0,	C,	0));
						#if DIM_M == 4
							STORETHISPS(mt.ma[3], reg_setr_ps(0,0,0,1));
						#endif
						return mt;
					}
					MT MT::RotationZ(RadF ang) {
						const float C = std::cos(ang.get()),
									S = std::sin(ang.get());
						MatT mt;
						STORETHISPS(mt.ma[0], reg_setr_ps(C,	S,	0,	0));
						STORETHISPS(mt.ma[1], reg_setr_ps(-S,	C,	0,	0));
						STORETHISPS(mt.ma[2], reg_setr_ps(0,	0,	1,	0));
						#if DIM_M == 4
							STORETHISPS(mt.ma[3], reg_setr_ps(0,0,0,1));
						#endif
						return mt;
					}
					MT MT::RotationAxis(const UVec3& axis, RadF ang) {
						const float C = std::cos(ang.get()),
									S = std::sin(ang.get()),
									RC = 1-C;
						MatT mt;
						const float M00 = C + Square(axis.x) * RC,
									M01 = axis.x * axis.y * RC + axis.z*S,
									M02 = axis.x * axis.z * RC - axis.y*S,
									M10 = axis.x * axis.y * RC - axis.z*S,
									M11 = C + Square(axis.y) * RC,
									M12 = axis.y * axis.z * RC + axis.x*S,
									M20 = axis.x * axis.z * RC + axis.y*S,
									M21 = axis.y * axis.z * RC - axis.x*S,
									M22 = C + Square(axis.z) * RC;
						STORETHISPS(mt.ma[0], reg_setr_ps(M00, M01, M02, 0));
						STORETHISPS(mt.ma[1], reg_setr_ps(M10, M11, M12, 0));
						STORETHISPS(mt.ma[2], reg_setr_ps(M20, M21, M22, 0));
						#if DIM_M == 4
							STORETHISPS(mt.ma[3], reg_setr_ps(0,0,0,1));
						#endif
						return mt;
					}
				#if DMIN == 4
					MT MT::_PerspectiveFov(RadF fov, float aspect, float nz, float fz, float coeff) {
						float h = 1.0f / std::tan(fov.get()/2),
								w = h / aspect,
								f0 = fz/(fz-nz),
								f1 = -nz*fz/(fz-nz);
						MatT ret;
						STORETHISPS(ret.ma[0], reg_setr_ps(w,0,0,0));
						STORETHISPS(ret.ma[1], reg_setr_ps(0,h,0,0));
						STORETHISPS(ret.ma[2], reg_setr_ps(0,0,f0,coeff));
						STORETHISPS(ret.ma[3], reg_setr_ps(0,0,f1*coeff,0));
						return ret;
					}
					MT MT::PerspectiveFovLH(RadF fov, float aspect, float nz, float fz) {
						return _PerspectiveFov(fov, aspect, nz, fz, 1);
					}
					MT MT::PerspectiveFovRH(RadF fov, float aspect, float nz, float fz) {
						return _PerspectiveFov(fov, aspect, nz, fz, -1);
					}
				#endif
			#endif
			bool MT::isEchelon() const {
				uint32_t clmFlagT = 0,
						clmFlagF = 0;
				int topClm = -1;
				bool bZero = false;
				for(int i=0 ; i<DIM_M ; i++) {
					int lcltop = -1;
					for(int j=0 ; j<DIM_N-1 ; j++) {
						float val = ma[i][j];
						if(std::fabs(val) >= FLOAT_EPSILON) {
							if(lcltop >= 0)
								clmFlagF |= 1<<j;
							else {
								lcltop = j;
								// 0でない成分を持つ全ての行について，先導成分が1である
								// 行の下に行くにつれて先導成分が右へずれていく
								if(topClm >= j || !(val-1.f >= FLOAT_EPSILON))
									return false;
								clmFlagT |= 1<<j;
							}
						}
					}
					if(lcltop < 0)
						bZero = true;
					else {
						// 0でない成分を持つすべての行は，0だけの行よりも先に来る
						if(bZero)
							return false;
						topClm = lcltop;
					}
				}
				// ある行の先導成分が第j列ならば，他の行の第j列は0である
				return !(clmFlagT & clmFlagF);
			}
			void MT::rowSwap(int r0, int r1) {
				reg128 xm0 = LOADTHIS(r0),
						xm1 = LOADTHIS(r1);
				STORETHIS(r1, xm0);
				STORETHIS(r0, xm1);
			}
			void MT::rowMul(int r0, float s) {
				STORETHIS(r0, reg_mul_ps(LOADTHIS(r0), reg_load1_ps(&s)));
			}
			void MT::rowMulAdd(int r0, float s, int r1) {
				reg128 xm0 = reg_mul_ps(LOADTHIS(r0), reg_load1_ps(&s)),
						xm1 = LOADTHIS(r1);
				STORETHIS(r1, reg_add_ps(xm0, xm1));
			}
			void MT::clmSwap(int c0, int c1) {
				setColumn(c1, getColumn(c0));
			}
			void MT::clmMul(int c0, float s) {
				auto clm = getColumn(c0);
				clm *= s;
				setColumn(c0, clm);
			}
			void MT::clmMulAdd(int c0, float s, int c1) {
				Column clm0 = getColumn(c0),
					clm1 = getColumn(c1);
				clm0 *= s;
				clm0 += clm1;
				setColumn(c1, clm0);
			}
			bool MT::isZeroRow(int n) const {
				return _mmIsZero(LOADTHISZ(n));
			}
			bool MT::isZeroRowEps(int n) const {
				return _mmIsZeroEps(LOADTHISZ(n));
			}
			bool MT::isZero() const {
				reg128 accum = reg_setzero_ps();
				#define ACCUM_ABS(dummy,n,dummy2)	reg_add_ps(accum, _mmAbsPs(LOADTHIS(n)));
				BOOST_PP_REPEAT(DIM_M, ACCUM_ABS, NOTHING)
				#undef ACCUM_ABS
				accum = reg_and_ps(accum, xmm_const::mask()[DIM_N]);
				return reg_movemask_ps(accum) == 0;
			}
			void MT::rowNormalize() {
				reg128 xm0 = LOADTHIS(0);
				for(int i=1 ; i<height ; i++)
					xm0 = reg_max_ps(xm0, LOADTHIS(i));
				RCP22BIT(xm0)
				for(int i=0 ; i<height ; i++)
					STORETHIS(i, reg_mul_ps(LOADTHIS(i), xm0));
			}
			int MT::rowReduce() {
				// rowNormalize();
				int rbase = 0,
					cbase = 0;
				for(;;) {
					// 行の先端が0でなく、かつ絶対値が最大の行を探す
					int idx = -1;
					float absmax = 0;
					for(int i=rbase ; i<DIM_M ; i++) {
						float v = std::fabs(ma[i][cbase]);
						if(absmax < v) {
							if(std::fabs(v) >= FLOAT_EPSILON) {
								absmax = v;
								idx = i;
							}
						}
					}
					if(idx < 0) {
						// 無かったので次の列へ
						++cbase;
						if(cbase == DIM_N) {
							// 終了
							break;
						}
						continue;
					}

					// 基準行でなければ入れ替え
					if(idx > rbase)
						rowSwap(idx, rbase);
					// 基点で割って1にする
					rowMul(rbase, spn::Rcp22Bit(ma[rbase][cbase]));
					ma[rbase][cbase] = 1;	// 精度の問題で丁度1にならない事があるので強制的に1をセット
					// 他の行の同じ列を0にする
					for(int i=0 ; i<DIM_M ; i++) {
						if(i==rbase)
							continue;

						float scale = -ma[i][cbase];
						rowMulAdd(rbase, scale, i);
						ma[i][cbase] = 0;	// 上と同じく精度の問題により0をセット
					}
					// 次の行,列へ移る
					++rbase;
					++cbase;
					if(rbase == DIM_M || cbase == DIM_N) {
						// 最後の行まで処理し終わるか，全て0の行しか無ければ終了
						break;
					}
				}
				return DIM_M - rbase;
			}
			#if DIM_M==DIM_N
				void MT::transpose() {
					*this = transposition();
				}
				float MT::calcDeterminant() const {
					#if DMIN == 2
						// 公式で計算
						return ma[0][0]*ma[1][1] - ma[0][1]*ma[1][0];
					#else
						float res = 0,
								s = 1;
						// 部分行列を使って計算
						for(int i=0 ; i<DIM_M ; i++) {
							auto mt = cutRC(0,i);
							res += ma[0][i] * mt.calcDeterminant() * s;
							s *= -1;
						}
						return res;
					#endif
				}
				bool MT::inversion(MatT& dst) const {
					assert(this != &dst);
					return inversion(dst, calcDeterminant());
				}
				bool MT::inversion(MatT& dst, float det) const {
					if(std::fabs(det) < FLOAT_EPSILON)
						return false;

					det = spn::Rcp22Bit(det);
					#if DIM_M==2
						dst.ma[0][0] = ma[1][1] * det;
						dst.ma[0][1] = -ma[1][0] * det;
						dst.ma[1][0] = -ma[0][1] * det;
						dst.ma[1][1] = ma[0][0] * det;
					#else
						const float c_val[2] = {1,-1};
						for(int i=0 ; i<DIM_M ; i++) {
							for(int j=0 ; j<DIM_N ; j++) {
								auto in_mat = cutRC(i,j);
								float in_det = in_mat.calcDeterminant();
								dst.ma[j][i] = c_val[(i+j)&1] * in_det * det;
							}
						}
					#endif
					return true;
				}
				bool MT::invert() {
					MT tmp(*this);
					bool b = inversion(tmp);
					if(b)
						*this = tmp;
					return b;
				}
				// -------- Luaへのエクスポート用 --------
				MT MT::addF(float s) const {
					return *this + s;
				}
				MT MT::addM(const MT& m) const {
					return *this * m;
				}
				MT MT::subF(float s) const {
					return *this - s;
				}
				MT MT::subM(const MT& m) const {
					return *this - m;
				}
				MT MT::mulF(float s) const {
					return *this * s;
				}
				MT MT::mulM(const MT& m) const {
					return *this * m;
				}
				MT MT::divF(float s) const {
					return *this / s;
				}
				MT MT::luaInvert() const {
					MT ret;
					inversion(ret);
					return ret;
				}
			#endif
			#if DMIN > 2
				//! 指定した行と列を省いた物を出力
				MatT<DIM_M-1,DIM_N-1,ALIGNB> MT::cutRC(int row, int clm) const {
					// TODO: 余力があったらマクロ展開
					MatT<height-1,width-1,ALIGNB> ret;
					// 左上
					for(int i=0 ; i<row ; i++) {
						for(int j=0 ; j<clm ; j++)
							ret.ma[i][j] = ma[i][j];
					}
					// 右上
					for(int i=0 ; i<row ; i++) {
						for(int j=clm+1 ; j<width ; j++)
							ret.ma[i][j-1] = ma[i][j];
					}
					// 左下
					for(int i=row+1 ; i<height ; i++) {
						for(int j=0 ; j<clm ; j++)
							ret.ma[i-1][j] = ma[i][j];
					}
					// 右下
					for(int i=row+1 ; i<height ; i++) {
						for(int j=clm+1 ; j<width ; j++)
							ret.ma[i-1][j-1] = ma[i][j];
					}
					return ret;
				}
			#endif
			MatT<DIM_N,DIM_M,ALIGNB> MT::transposition() const {
				MatT<DIM_N,DIM_M,ALIGNB> ret;
				#if DMIN == 2
					// DMIN == 2なら直接入れ替えたほうが速い
					ret.ma[0][0] = ma[0][0];
					ret.ma[0][1] = ma[1][0];
					ret.ma[1][0] = ma[0][1];
					ret.ma[1][1] = ma[1][1];
				#else
					// DMIN >= 3なら4x4行列として処理
					// M*4行列 = レジスタM本
					reg128	xm[4], xmt[4];
					#define LOADROWE(n) BOOST_PP_IF(BOOST_PP_GREATER_EQUAL(n, DIM_M), xmm_const::matI()[n], LOADTHIS(n))
					#define BOOST_PP_LOCAL_MACRO(n)	xm[n] = LOADROWE(n);
					#define BOOST_PP_LOCAL_LIMITS (0,BOOST_PP_SUB(DMAX, 1))
					#include BOOST_PP_LOCAL_ITERATE()

					xmt[0] = reg_unpacklo_ps(xm[0], xm[2]);
					xmt[1] = reg_unpacklo_ps(xm[1], xm[3]);
					xmt[2] = reg_unpackhi_ps(xm[0], xm[2]);
					xmt[3] = reg_unpackhi_ps(xm[1], xm[3]);

					xm[0] = reg_unpacklo_ps(xmt[0], xmt[1]);
					xm[1] = reg_unpackhi_ps(xmt[0], xmt[1]);
					xm[2] = reg_unpacklo_ps(xmt[2], xmt[3]);
					xm[3] = reg_unpackhi_ps(xmt[2], xmt[3]);
					#define BOOST_PP_LOCAL_MACRO(n) STORETHISPS(ret.ma[n], xm[n]);
					#define BOOST_PP_LOCAL_LIMITS (0,BOOST_PP_SUB(DIM_N,1))
					#include BOOST_PP_LOCAL_ITERATE()
					#undef LOADROWE
				#endif
				return ret;
			}
			/*	Pseudo-code:
				MatT<n0,n1,align> MT::convert##align##n0##n1() const {
					MatT<n0,n1,align> ret;
					// <Repeat by ITR_COPY>
					for(int i=0 ; i<n0 ; i++) {
						if(i < DIM_M)
							STOREPS_(A)##n1(ret.ma[i], LOADTHIS(i));
						else
							STOREPS_(A)##n1(ret.ma[i], xmm_const::matI()[i]);
					}
					return ret;
				}
				void MT::convert(MatT<n0,n1,align>& dst) const {
					 dst = convert##align##n0##n1();
				}
			*/
			#define ITR_COPY_I(n,dim,align)	\
				BOOST_PP_CAT( \
					BOOST_PP_CAT( \
						STOREPS_, \
						AFLAG(align) \
					), \
					dim \
				) \
				(ret.ma[n], \
					BOOST_PP_IF( \
						BOOST_PP_LESS(n,DIM_M), \
						LOADTHISI(n), \
						xmm_const::matI()[n] \
					) \
				);
			#define ITR_COPY(z,n,data)		ITR_COPY_I(n, BOOST_PP_SEQ_ELEM(0,data), BOOST_PP_SEQ_ELEM(1,data))
			#define DEF_CONV(n0,n1,align)	\
				MatT<n0,n1,BOOLNIZE(align)> MT::BOOST_PP_CAT( \
					BOOST_PP_CAT( \
						BOOST_PP_CAT(convert, AFLAG(align)), \
						n0 \
					), \
					n1)() const { \
				MatT<n0,n1,BOOLNIZE(align)> ret; \
				BOOST_PP_REPEAT(n0, ITR_COPY, (n1)(align)) \
				return ret; } \
				void MT::convert(MatT<n0,n1,BOOLNIZE(align)>& dst) const { \
					dst = BOOST_PP_CAT( \
							BOOST_PP_CAT( \
								BOOST_PP_CAT(convert, AFLAG(align)), \
								n0 \
							), \
							n1 \
						)(); }
			BOOST_PP_REPEAT(LEN_SEQ, DEF_CONV_ITR, DEF_CONV)
			#undef ITR_COPY_I
			#undef ITR_COPY
		}
		#elif BOOST_PP_FRAME_FLAGS(1) == 2
		namespace spn {
			#define DEF_MULSELF *this = *this * *this;
			MT& MT::mul_self() {
				BOOST_PP_IF(
					BOOST_PP_EQUAL(DIM_M, DIM_N),
					DEF_MULSELF,
					NOTHING
				)
				return *this;
			}
			#undef DEF_MULSELF
			// 行列の積算
			/*	Pseudo-code:
				MatT<DIM_M, n1, ALIGNB> MT::operator * (const MatT<n0,n1,align>& m) const {
					ALIGN16 MatT<DIM_M, n1, ALIGNB> ret;
					// <Repeat by MUL_OUTER>
					for(int i=0 ; i<n0 ; i++) {
						reg128 tm = LOADTHISI(i);
						reg128 accum = reg_mul_ps(reg_shuffle_ps(tm, tm, _REG_SHUFFLE(0,0,0,0)), LOADPS_I(A)n1(m.ma[0], 0));
						// <Repeat by MUL_INNER>
						for(int j=1 ; j<DIM_N ; j++)
							accum = reg_add_ps(accum, reg_mul_ps(reg_shuffle_ps(tm, tm, _REG_SHUFFLE(j,j,j,j)), LOADPS_I(A)n1(m.ma[j], j)));
						STOREPS_(A)n1(ret.ma[i], accum);
					}
					return ret;
				}
			*/
			// 対応する行列(サイズ)以外は何もコードを出力しない
			#define DEF_MUL0(n0,n1,align) \
				BOOST_PP_IF( \
					BOOST_PP_EQUAL(DIM_N, n0), \
					DEF_MUL, \
					NOTHING_ARG \
				)(n0,n1,align)
			#define DEF_MUL(n0,n1,align)	MatT<DIM_M, n1, ALIGNB> MT::operator * (const MatT<n0,n1,BOOLNIZE(align)>& m) const {\
				ALIGN16 MatT<DIM_M, n1, ALIGNB> ret; \
				BOOST_PP_REPEAT( \
					DIM_M, \
					MUL_OUTER, \
					(BOOST_PP_CAT( \
						BOOST_PP_IF( \
							align, \
							A, \
							NOTHING \
						), \
						n1 \
					))(n0) \
				) \
				return ret; \
			}
			/*	Inner(z,n, AUn1)
				accum = reg_add_ps(accum, reg_mul_ps(reg_shuffle_ps(tm, tm, _REG_SHUFFLE(n,n,n,n)), LOADPS_I(A)n1(m.ma[n], n)));
			*/
			#define MUL_INNER(z,n,AUn1)	accum = reg_add_ps( \
													accum, \
													reg_mul_ps( \
														reg_shuffle_ps(tm, tm, _REG_SHUFFLE(n,n,n,n)), \
														LOADPS_I##AUn1(m.ma[n], n) \
													) \
												);
			/*	Outer(z,i, AUn1_n0)
				reg128 tm = LOADTHISI(i);
				reg128 accum = reg_mul_ps(reg_shuffle_ps(tm, tm, _REG_SHUFFLE(0,0,0,0)), LOADPS_I(A)n1(m.ma[0], 0));
				for(int j=1 ; j<AUn1_n0[1] ; j++)
					Inner(j, AUn1_n0[0]);
				STOREPS_(A)AUn1_n0[0](ret.ma[i], accum);
			*/
			#define MUL_OUTER(z,n,AUn1_n0) { \
				reg128 tm = LOADTHISI(n); \
				reg128 accum = reg_mul_ps( \
								reg_shuffle_ps(tm, tm, _REG_SHUFFLE(0,0,0,0)), \
								BOOST_PP_CAT( \
									LOADPS_I, \
									BOOST_PP_SEQ_ELEM(0,AUn1_n0) \
								)(m.ma[0], 0) \
							); \
				BOOST_PP_REPEAT_FROM_TO( \
					1, \
					BOOST_PP_SEQ_ELEM(1,AUn1_n0), \
					MUL_INNER, \
					BOOST_PP_SEQ_ELEM(0,AUn1_n0) \
				) \
				BOOST_PP_CAT(STOREPS_, BOOST_PP_SEQ_ELEM(0, AUn1_n0))(ret.ma[n], accum); }
			BOOST_PP_REPEAT(LEN_SEQ, DEF_CONV_ITR, DEF_MUL0)
			#undef DEF_MUL0
			#undef DEF_MUL
		}
		#else
		namespace spn {
			// 自分との積算でなければ上書き演算
			/*	Pseudo-code:
				MT& MT::operator *= (const MatT<n0,n1,align>& m) {
					// 自分との積算ならば *this = *this * *this; の形で計算
					if((uintptr_t)&m == (uintptr_t)this)
						return *this = *this * *this;
					// <Repeat by MUL_OUTER2>
					for(int i=0 ; i<DIM_M ; i++) {
						reg128 tm = LOADTHIS(n);
						reg128 accum = reg_mul_ps(reg_shuffle_ps(tm,tm, _REG_SHUFFLE(0,0,0,0)), LOADTHIS(0));
						// <Repeat by MUL_INNER2>
						for(int j=0 ; j<DIM_M ; j++)
							reg_add_ps(accum, reg_mul_ps(reg_shuffle_ps(tm,tm, _REG_SHUFFLE(j,j,j,j)), LOADTHIS(j));
						STORETHIS(j,accum);
					}
					return *this;
				}
			*/
			#define DEF_MULE0(n0,n1,align) \
				BOOST_PP_IF( \
					BOOST_PP_AND( \
						BOOST_PP_EQUAL(DIM_N, n0), \
						BOOST_PP_EQUAL(n1, DIM_N) \
					), \
					DEF_MULE, \
					NOTHING_ARG \
				)(n0,n1,align)
			#define DEF_MULE(n0,n1,align) \
				MT& MT::operator *= (const MatT<n0,n1,BOOLNIZE(align)>& m) { \
					if((intptr_t)&m == (intptr_t)this) return mul_self(); \
						BOOST_PP_REPEAT( \
							DIM_M, MUL_OUTER2, \
							BOOST_PP_IF(align, NOTHING, U) \
						) \
					return *this; }
			#define MUL_INNER2(z,n,AU) \
				accum = reg_add_ps( \
					accum, \
					reg_mul_ps(reg_shuffle_ps(tm,tm, _REG_SHUFFLE(n,n,n,n)), LOADPS##AU(m.ma[n])) \
				);
			#define MUL_OUTER2(z,n,AU) { \
				reg128 tm = LOADTHIS(n); \
				reg128 accum = reg_mul_ps(reg_shuffle_ps(tm, tm, _REG_SHUFFLE(0,0,0,0)), LOADPS##AU(m.ma[0])); \
				BOOST_PP_REPEAT_FROM_TO(1,DIM_N, MUL_INNER2, AU) \
				STORETHISPS(ma[n], accum); }
			BOOST_PP_REPEAT(LEN_SEQ, DEF_CONV_ITR, DEF_MULE0)
			#undef DEF_MULE0
			#undef DEF_MULE
			#undef DEF_INNER2
			#undef DEF_OUTER2

			// 他の行列やベクトルと計算するメソッドを定義
			/*	Pseudo-code:
				template <>
				VecT<DIM_N,ALIGNB> operator * (const VecT<DIM_N,align>& v) const {
					auto tmpM = transposition();
					return v * tmpM;
				}
			*/
			#define DEF_MULOP(z,align,dummy) \
				template <> VecT<DIM_N,ALIGNB> MT::operator * (const VecT<DIM_N,BOOLNIZE(align)>& v) const { \
					auto tmpM = BOOST_PP_CAT( \
									BOOST_PP_CAT( \
										BOOST_PP_CAT( \
											transposition().convert, \
											AFLAG(ALIGN) \
										), \
										DIM_N \
									), \
									DIM_N \
								)(); \
					return v * tmpM; }
			BOOST_PP_REPEAT(1, DEF_MULOP, NOTHING)
			#undef DEF_MULOP
			#if DIM_M==DIM_N
				VecT<DIM_N,MT::align> MT::mulV(const VecT<DIM_N,align>& v) const {
					auto tmpM = transposition();
					return v * tmpM;
				}
			#endif
		}
	#endif
	#include "local_unmacro.hpp"
	#undef TUP
	#undef DIM_M
	#undef DIM_N
	#undef DIM
	#undef ALIGN
	#undef ALIGNB
	#undef ALIGN16
	#undef MT
	#undef DMAX
	#undef DMIN
	#undef DMUL
	#undef DIMTUPLE
	#undef DEF_CONV_ITR
	#undef DEFINE_MATRIX
#endif
