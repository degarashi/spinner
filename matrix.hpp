//! 行列クラス基底ヘッダ
/*! 他行列との演算を除いたメソッドを定義 */
#if !BOOST_PP_IS_ITERATING
	#if !defined(MATRIX_H_) || INCLUDE_MATRIX_LEVEL >= 1
		#define MATRIX_H_
		#define BOOST_PP_VARIADICS 1
		#include <boost/preprocessor.hpp>
		#include <cstring>
		#include <stdexcept>
		#include "spn_math.hpp"
		
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
			struct ALIGN16 MatT<DIM_M, DIM_N, ALIGNB> : MatBase {
				using Row = VecT<DIM_N, ALIGNB>;			//!< 行を表すベクトル型
				using Column = VecT<DIM_M, false>;			//!< 列を表すベクトル型
				using RowE = VecT<4, ALIGNB>;				//!< 4要素の行ベクトル型
				using AMat = MatT<DIM_M, DIM_N, true>;
				using UMat = MatT<DIM_M, DIM_N, false>;
				using UVec2 = VecT<2,false>;
				using UVec3 = VecT<3,false>;
				
				enum { width=DIM_N,
						height=DIM_M };
				const static int PaddingedSize = BOOST_PP_IF(ALIGN, 4, DIM_N),
								AlignedSize = DIM_M*4-(4-DIM_N),
								UnAlignedSize = DIM_M*DIM_N;
				union {
					//! 容量確保用
					float	data[BOOST_PP_IF(ALIGN, AlignedSize, UnAlignedSize)];
					//! 2次元配列アクセス用
					float	ma[0][PaddingedSize];
				};
				
				// -------------------- ctor --------------------
				MatT() = default;
				BOOST_PP_IF(ALIGN, NOTHING, explicit) MatT(const AMat& m);
				BOOST_PP_IF(ALIGN, explicit, NOTHING) MatT(const UMat& m);
				
				#define DIAGONAL2(z,n1,n0)		ma[n0][n1] = BOOST_PP_IF(BOOST_PP_EQUAL(n0,n1), s, 0);
				#define DIAGONAL(z,n,data)		BOOST_PP_REPEAT_##z(DIM_N, DIAGONAL2, n)
				MatT(float s, _TagDiagonal) {
					BOOST_PP_REPEAT(DIM_M, DIAGONAL, NOTHING)
				}
				#define ALLSET(z,n,data)	STORETHIS(n, data);
				MatT(float s, _TagAll) {
					__m128 r = _mm_load1_ps(&s);
					BOOST_PP_REPEAT(DIM_M, ALLSET, r)
				}
				MatT(_TagIdentity): MatT(1.f, TagDiagonal) {}
				
				#define SETARRAY(z,n,src)	STORETHIS(n, _mm_loadu_ps(src)); src += DIM_N;
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
				#define SET_ARGS1(z,n,data)	ma[n][n] = BOOST_PP_CAT(data,n);
				//! 対角線上
				MatT(BOOST_PP_SEQ_ENUM(BOOST_PP_REPEAT(DMIN, DEF_ARGS, f))) {
					BOOST_PP_REPEAT(DMIN, SET_ARGS1, f)
				}
				
				// -------------------- query values --------------------
				const Row& getRow(int n) const {
					if(n >= height)
						return *reinterpret_cast<const Row*>(cs_matI[n]);
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
							return RowE(xmm_matI[n]);
						return RowE(_mm_or_ps(_mm_and_ps(LOADTHIS(n), xmm_mask[n]), xmm_matI[n]));
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
						return Column(xmm_matI[n]);
					return Column(BOOST_PP_SEQ_ENUM(BOOST_PP_REPEAT(DIM_M, SET_COLUMN, n)));
				}
				
				#define DEF_GETROW(z,n,data)	BOOST_PP_CAT(const Row& getRow, n)() const { return *reinterpret_cast<const Row*>(ma[n]); }
				//! マクロからのアクセス用: 行毎に個別のメンバ関数を用意
				/*! マクロのColumn取得は多分、不要 */
				BOOST_PP_REPEAT(DIM_M, DEF_GETROW, NOTHING)
				
				// -------------------- operators --------------------
				#define FUNC(z,n,func)		STOREPS(ma[n], func(LOADPS(ma[n]), r0));
				#define FUNC2(z,n,func)		STOREPS(ret.ma[n], func(LOADPS(ma[n]), r0));
				#define DEF_OP(op, func) MatT& operator BOOST_PP_CAT(op,=) (float s) { \
					__m128 r0 = _mm_load1_ps(&s); \
					BOOST_PP_REPEAT(DIM_M, FUNC, func) \
					return *this; } \
					MatT operator op (float s) const { \
						MatT ret; \
						__m128 r0 = _mm_load1_ps(&s); \
						BOOST_PP_REPEAT(DIM_M, FUNC2, func) \
						return ret; }

				DEF_OP(+, _mm_add_ps)
				DEF_OP(-, _mm_sub_ps)
				DEF_OP(*, _mm_mul_ps)
				DEF_OP(/, _mmDivPs)
				
				#define FUNC3(z,n,func)		STOREPS(ma[n], func(LOADPS(m.ma[n]), LOADPS(ma[n])));
				#define FUNC4(z,n,func)		STOREPS(ret.ma[n], func(LOADPS(m.ma[n]), LOADPS(ma[n])));
				#define DEF_OPM(op, func) MatT& operator BOOST_PP_CAT(op,=) (const MatT& m) { \
					BOOST_PP_REPEAT(DIM_M, FUNC3, func) \
					return *this; } \
				MatT operator op (const MatT& m) const { \
					MatT ret; \
					BOOST_PP_REPEAT(DIM_M, FUNC4, func) \
					return ret; }

				DEF_OPM(+, _mm_add_ps)
				DEF_OPM(-, _mm_sub_ps)
				
				// -------------------- others --------------------
				// 行列拡張Get = Mat::getRowE()
				void identity();
				static MatT Scaling(BOOST_PP_SEQ_ENUM(BOOST_PP_REPEAT(DMIN, DEF_ARGS, f)));
				#if DMIN == 2
					//! 2D回転
					static MatT Rotation(float ang);
					#if DIM_M == 3
						//! 2次元移動
						static MatT Translation(const UVec2& v);
					#endif
				#elif DMIN >= 3
						//! X軸周りの回転
						static MatT RotationX(float ang);
						//! Y軸周りの回転
						static MatT RotationY(float ang);
						//! Z軸周りの回転
						static MatT RotationZ(float ang);
						//! 任意軸周りの回転
						static MatT RotationAxis(const UVec3& axis, float ang);
					#if DIM_M == 4
						//! 3次元移動
						static MatT Translation(const UVec3& v);
						static MatT LookAtLH(const UVec3& pos, const UVec3& at, const UVec3& up);
						static MatT LookDirLH(const UVec3& pos, const UVec3& dir, const UVec3& up);
						static MatT LookAtRH(const UVec3& pos, const UVec3& at, const UVec3& up);
						static MatT LookDirRH(const UVec3& pos, const UVec3& dir, const UVec3& up);
					#endif
					#if DIM_M==4 && DIM_N==4
						//! 透視変換行列
						static MatT PerspectiveFovLH(float fov, float aspect, float nz, float fz);
						static MatT PerspectiveFovRH(float fov, float aspect, float nz, float fz);
						static MatT _PerspectiveFov(float fov, float aspect, float nz, float fz, float coeff);
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
				#endif
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
				#define DEF_CONV(n0,n1,align)	MatT<n0,n1,BOOLNIZE(align)> BOOST_PP_CAT(BOOST_PP_CAT(BOOST_PP_CAT(convert,AFLAG(align)), n0),n1)() const;
				BOOST_PP_REPEAT(LEN_SEQ, DEF_CONV_ITR, DEF_CONV)

				//! 行列との積算 (3 operands)
				#define DEF_MUL(n0,n1,align)	MatT<DIM_M, n1, ALIGNB> operator * (const MatT<n0,n1,BOOLNIZE(align)>& m) const;
				BOOST_PP_REPEAT(LEN_SEQ, DEF_CONV_ITR, DEF_MUL)
				//! 行列との積算 (2 operands)
				#define DEF_MULE(n0,n1,align)	MatT& operator *= (const MatT<n0,n1,BOOLNIZE(align)>& m);
				BOOST_PP_REPEAT(LEN_SEQ, DEF_CONV_ITR, DEF_MULE)
				
				//! 行列との積算 (右から掛ける)
				/*! 列ベクトルとして扱う = ベクトルを転置して左から行ベクトルを掛ける */
				template <bool A>
				VecT<DIM_N,ALIGNB> operator * (const VecT<DIM_N,A>& v) const;
			};
			// 使いやすいように別名を定義 ex. MatT<2,2,true> = AMat22
			using BOOST_PP_CAT(
					BOOST_PP_CAT(
						BOOST_PP_CAT(AFLAG(ALIGN),Mat)
						,DIM_M)
					,DIM_N)
					= MatT<DIM_M,DIM_N,ALIGNB>;
		}
	#elif BOOST_PP_FRAME_FLAGS(1) == 1
		namespace spn {
			MT::MatT(const AMat& m) {
				for(int i=0 ; i<DIM_M ; i++)
					STORETHIS(i, LOADPS(m.ma[i]));
			}
			MT::MatT(const UMat& m) {
				for(int i=0 ; i<DIM_M ; i++)
					STORETHIS(i, LOADPSU(m.ma[i]));
			}
			// 行列拡張Get = Mat::getRowE()
			void MT::identity() {
				*this = MatT(1.0f, TagDiagonal);
			}
			#define SET_ARGS2(z,n,data) (BOOST_PP_CAT(data, n))
			MT MT::Scaling(BOOST_PP_SEQ_ENUM(BOOST_PP_REPEAT(DMIN, DEF_ARGS, f))) {
				return MatT(BOOST_PP_SEQ_ENUM(BOOST_PP_REPEAT(DMIN, SET_ARGS2, f)));
			}
			#if DMIN == 2
				MT MT::Rotation(float ang) {
					float s = std::sin(ang),
							c = std::cos(ang);
					MatT mt(TagIdentity);
					mt.ma[0][0] = s;
					mt.ma[0][1] = c;
					mt.ma[1][0] = c;
					mt.ma[1][1] = -s;
					return mt;
				}
				#if DIM_M == 3
					MT MT::Translation(const VecT<2,false>& v) {
						MatT mt(TagIdentity);
						mt.ma[0][2] = v.x;
						mt.ma[1][2] = v.y;
						return mt;
					}
				#endif
			#elif DMIN >= 3
					MT MT::RotationX(float ang) {
						float C = std::cos(ang),
							S = std::sin(ang);
							
						MatT mt;
						STORETHISPS(mt.ma[0], _mm_setr_ps(1,0,0,0));
						STORETHISPS(mt.ma[1], _mm_setr_ps(0,C,-S,0));
						STORETHISPS(mt.ma[2], _mm_setr_ps(0,S,C,0));
						#if DIM_M == 4
							STORETHISPS(mt.ma[3], _mm_setr_ps(0,0,0,1));
						#endif
						return mt;
					}
					MT MT::RotationY(float ang) {
						float C = std::cos(ang),
							S = std::sin(ang);
						MatT mt;
						STORETHISPS(mt.ma[0], _mm_setr_ps(C,0,S,0));
						STORETHISPS(mt.ma[1], _mm_setr_ps(0,1,0,0));
						STORETHISPS(mt.ma[2], _mm_setr_ps(-S,0,C,0));
						#if DIM_M == 4
							STORETHISPS(mt.ma[3], _mm_setr_ps(0,0,0,1));
						#endif
						return mt;
					}
					MT MT::RotationZ(float ang) {
						float C = std::cos(ang),
							S = std::sin(ang);
						MatT mt;
						STORETHISPS(mt.ma[0], _mm_setr_ps(C,-S,0,0));
						STORETHISPS(mt.ma[1], _mm_setr_ps(S,C,0,0));
						STORETHISPS(mt.ma[2], _mm_setr_ps(0,0,1,0));
						#if DIM_M == 4
							STORETHISPS(mt.ma[3], _mm_setr_ps(0,0,0,1));
						#endif
						return mt;
					}
					MT MT::RotationAxis(const UVec3& axis, float ang) {
						float C = std::cos(ang),
							S = std::sin(ang),
							RC = 1-C;
						MatT mt;
						STORETHISPS(mt.ma[0], _mm_setr_ps(C+Square(axis.x)*RC,
														axis.x * axis.y * RC + axis.z*S,
														axis.x * axis.z * RC + axis.y*S, 0));
						STORETHISPS(mt.ma[1], _mm_setr_ps(axis.x * axis.y * RC - axis.z*S,
														C + Square(axis.y) * RC,
														 axis.y * axis.z * RC + axis.x*S, 0));
						STORETHISPS(mt.ma[2], _mm_setr_ps(axis.x * axis.z * RC + axis.y*S,
														axis.y * axis.z * RC + axis.x*S,
														C + Square(axis.z) * RC, 0));
						#if DIM_M == 4
							STORETHISPS(mt.ma[3], _mm_setr_ps(0,0,0,1));
						#endif
						return mt;
					}
				#if DIM_M == 4
					MT MT::Translation(const UVec3& v) {
						MatT mt(TagIdentity);
						mt.ma[0][3] = v.x;
						mt.ma[1][3] = v.y;
						mt.ma[2][3] = v.z;
						return mt;
					}
					MT MT::LookAtLH(const UVec3& pos, const UVec3& at, const UVec3& up) {
						return LookDirLH(pos, (at-pos).normalization(), up);
					}
					MT MT::LookDirLH(const UVec3& pos, const UVec3& dir, const UVec3& up) {
						AVec3 xA(up % dir);
						xA.normalize();
						
						MatT ret;
						STORETHISPS(ret.ma[0], _mm_setr_ps(xA.x, up.x, dir.x, 0));
						STORETHISPS(ret.ma[1], _mm_setr_ps(xA.y, up.y, dir.y, 0));
						STORETHISPS(ret.ma[2], _mm_setr_ps(xA.z, up.z, dir.z, 0));
						STORETHISPS(ret.ma[3], _mm_setr_ps(-pos.dot(xA), -pos.dot(up), -pos.dot(dir), 1));
						return ret;
					}
					MT MT::LookAtRH(const UVec3& pos, const UVec3& at, const UVec3& up) {
						return LookDirLH(pos, (pos-at).normalization(), up);
					}
					MT MT::LookDirRH(const UVec3& pos, const UVec3& dir, const UVec3& up) {
						return LookDirLH(pos, -dir, up);
					}
				#endif
				#if DIM_M==4 && DIM_N==4
					MT MT::_PerspectiveFov(float fov, float aspect, float nz, float fz, float coeff) {
						float h = 1.0f / std::tan(fov/2),
								w = h / aspect,
								f0 = fz/(fz-nz),
								f1 = -nz*fz/(fz-nz);
						MatT ret;
						STORETHISPS(ret.ma[0], _mm_setr_ps(w,0,0,0));
						STORETHISPS(ret.ma[1], _mm_setr_ps(0,h,0,0));
						STORETHISPS(ret.ma[2], _mm_setr_ps(0,0,f0,coeff));
						STORETHISPS(ret.ma[3], _mm_setr_ps(0,0,f1*coeff,0));
						return ret;
					}
					MT MT::PerspectiveFovLH(float fov, float aspect, float nz, float fz) {
						return _PerspectiveFov(fov, aspect, nz, fz, 1);
					}
					MT MT::PerspectiveFovRH(float fov, float aspect, float nz, float fz) {
						return _PerspectiveFov(fov, aspect, nz, fz, 1);
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
				__m128 xm0 = LOADPS(ma[r0]),
						xm1 = LOADPS(ma[r1]);
				STOREPS(ma[r1], xm0);
				STOREPS(ma[r0], xm1);
			}
			void MT::rowMul(int r0, float s) {
				STOREPS(ma[r0], _mm_mul_ps(LOADPS(ma[r0]), _mm_load1_ps(&s)));
			}
			void MT::rowMulAdd(int r0, float s, int r1) {
				__m128 xm0 = _mm_mul_ps(LOADPS(ma[r0]), _mm_load1_ps(&s)),
						xm1 = LOADPS(ma[r1]);
				STORETHIS(r1, _mm_add_ps(xm0, xm1));
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
				__m128 xm0 = LOADPSZ(ma[n]);
				__m128 xm1 = _mm_cmplt_ps(xm0, xmm_epsilon),
						xm2 = _mm_cmpnle_ps(xm0, xmm_epsilonM);
				xm1 = _mm_or_ps(xm1, xm2);
				return _mm_movemask_ps(xm1) == 0;
			}
			bool MT::isZero() const {
				__m128 accum = _mm_setzero_ps();
				#define ACCUM_ABS(dummy,n,dummy2)	_mm_add_ps(accum, _mmAbsPs(LOADTHIS(n)));
				BOOST_PP_REPEAT(DIM_M, ACCUM_ABS, NOTHING)
				accum = _mm_and_ps(accum, xmm_mask[DIM_N]);
				return _mm_movemask_ps(accum) == 0;
			}
			void MT::rowNormalize() {
				__m128 xm0 = LOADPS(ma[0]);
				for(int i=1 ; i<height ; i++)
					xm0 = _mm_max_ps(xm0, LOADPS(ma[i]));
				RCP22BIT(xm0)
				for(int i=0 ; i<height ; i++)
					STOREPS(ma[i], _mm_mul_ps(LOADTHIS(i), xm0));
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
					rowMul(rbase, _sseRcp22Bit(ma[rbase][cbase]));
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
					return inversion(dst, calcDeterminant());
				}
				bool MT::inversion(MatT& dst, float det) const {
					if(std::fabs(det) < FLOAT_EPSILON)
						return false;
					
					det = _sseRcp22Bit(det);
					#if DIM_M==2
						dst.ma[0][0] = ma[1][1] * det;
						dst.ma[0][1] = -ma[1][0] * det;
						dst.ma[1][0] = -ma[0][1] * det;
						dst.ma[1][1] = ma[0][0] * det;
					#else
						const float c_val[2] = {1,-1};
						for(int i=0 ; i<DIM_M ; i++) {
							for(int j=0 ; j<DIM_N ; j++) {
								auto in_mat = cutRC(j,i);
								float in_det = in_mat.calcDeterminant();
								dst.ma[i][j] = c_val[(i+j)&1] * in_det * det;
							}
						}
					#endif
					return true;
				}
				bool MT::invert() {
					return inversion(*this);
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
					__m128	xm[4], xmt[4];
					#define LOADROWE(n) BOOST_PP_IF(BOOST_PP_GREATER_EQUAL(n, DIM_M), xmm_matI[n], LOADTHIS(n))
					#define BOOST_PP_LOCAL_MACRO(n)	xm[n] = LOADROWE(n);
					#define BOOST_PP_LOCAL_LIMITS (0,DMAX)
					#include BOOST_PP_LOCAL_ITERATE()
					
					xmt[0] = _mm_unpacklo_ps(xm[0], xm[2]);
					xmt[1] = _mm_unpacklo_ps(xm[1], xm[3]);
					xmt[2] = _mm_unpackhi_ps(xm[0], xm[2]);
					xmt[3] = _mm_unpackhi_ps(xm[1], xm[3]);

					xm[0] = _mm_unpacklo_ps(xmt[0], xmt[1]);
					xm[1] = _mm_unpackhi_ps(xmt[0], xmt[1]);
					xm[2] = _mm_unpacklo_ps(xmt[2], xmt[3]);
					xm[3] = _mm_unpackhi_ps(xmt[2], xmt[3]);
					#define BOOST_PP_LOCAL_MACRO(n) STOREPS(ret.ma[n], xm[n]);
					#define BOOST_PP_LOCAL_LIMITS (0,DIM_N)
					#include BOOST_PP_LOCAL_ITERATE()
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
							STOREPS_(A)##n1(ret.ma[i], xmm_matI[i]);
					}
					return ret;
				}
			*/
			#define ITR_COPY_I(n,dim,align)	BOOST_PP_CAT(BOOST_PP_CAT(STOREPS_, AFLAG(align)), dim)(ret.ma[n], BOOST_PP_IF(BOOST_PP_LESS(n,DIM_M), \
												LOADTHISI(n), \
												xmm_matI[n]));
			#define ITR_COPY(z,n,data)		ITR_COPY_I(n, BOOST_PP_SEQ_ELEM(0,data), BOOST_PP_SEQ_ELEM(1,data))
			#define DEF_CONV(n0,n1,align)	MatT<n0,n1,BOOLNIZE(align)> MT::BOOST_PP_CAT( \
					BOOST_PP_CAT( \
						BOOST_PP_CAT(convert, AFLAG(align)), \
						n0), \
					n1)() const { \
 				MatT<n0,n1,BOOLNIZE(align)> ret; \
				BOOST_PP_REPEAT(n0, ITR_COPY, (n1)(align)) \
				return ret; }
			BOOST_PP_REPEAT(LEN_SEQ, DEF_CONV_ITR, DEF_CONV)
			
			// 行列の積算
			/*	Pseudo-code:
				MatT<DIM_M, n1, ALIGNB> MT::operator * (const MatT<n0,n1,align>& m) const {
					ALIGN16 MatT<DIM_M, n1, ALIGNB> ret;
					// <Repeat by MUL_OUTER>
					for(int i=0 ; i<n0 ; i++) {
						__m128 tm = LOADTHIS(i);
						__m128 accum = _mm_mul_ps(_mm_shuffle_ps(tm, tm, _MM_SHUFFLE(0,0,0,0)), LOADPS(m.ma[0]));
						// <Repeat by MUL_INNER>
						for(int j=0 ; j<DIM_N ; j++)
							_mm_add_ps(accum, _mm_mul_ps(_mm_shuffle_ps(tm, tm, _MM_SHUFFLE(j,j,j,j)), LOADPS(m.ma[j])));
						STOREPS_(A)4(ret.ma[i], accum);
					}
					return ret;
				}
			*/
			#define DEF_MUL(n0,n1,align)	MatT<DIM_M, n1, ALIGNB> MT::operator * (const MatT<n0,n1,BOOLNIZE(align)>& m) const {\
				ALIGN16 MatT<DIM_M, n1, ALIGNB> ret; \
				BOOST_PP_REPEAT(n0, MUL_OUTER, BOOST_PP_IF(align, NOTHING, U)) \
				return ret; \
			}
			#define MUL_INNER(z,n,data)	_mm_add_ps(accum, _mm_mul_ps(_mm_shuffle_ps(tm, tm, _MM_SHUFFLE(n,n,n,n)), LOADPS(m.ma[n])));
			#define MUL_OUTER(z,n,AU)	{ __m128 tm = LOADTHIS(n); \
				__m128 accum = _mm_mul_ps(_mm_shuffle_ps(tm, tm, _MM_SHUFFLE(0,0,0,0)), LOADPS##AU(m.ma[0])); \
				BOOST_PP_REPEAT_FROM_TO(1,DIM_N, MUL_INNER, NOTHING) \
				BOOST_PP_CAT(STOREPS_, BOOST_PP_CAT(AFLAG(ALIGN),4))(ret.ma[n], accum); }
			BOOST_PP_REPEAT(LEN_SEQ, DEF_CONV_ITR, DEF_MUL)
			
			// 自分との積算でなければ上書き演算
			/*	Pseudo-code:
				MT& MT::operator *= (const MatT<n0,n1,align>& m) {
					// 自分との積算ならば *this = *this * *this; の形で計算
					if((uintptr_t)&m == (uintptr_t)this)
						return *this = *this * *this;
					// <Repeat by MUL_OUTER2>
					for(int i=0 ; i<DIM_M ; i++) {
						__m128 tm = LOADTHIS(n);
						__m128 accum = _mm_mul_ps(_mm_shuffle_ps(tm,tm, _MM_SHUFFLE(0,0,0,0)), LOADTHIS(0));
						// <Repeat by MUL_INNER2>
						for(int j=0 ; j<DIM_M ; j++)
							_mm_add_ps(accum, _mm_mul_ps(_mm_shuffle_ps(tm,tm, _MM_SHUFFLE(j,j,j,j)), LOADTHIS(j));
						STORETHIS(j,accum);
					}
					return *this;
				}
			*/
			#define DEF_MULE(n0,n1,align)	MT& MT::operator *= (const MatT<n0,n1,BOOLNIZE(align)>& m) { \
						if((intptr_t)&m == (intptr_t)this) return *this = *this * *this; \
						BOOST_PP_REPEAT(DIM_M, MUL_OUTER2, BOOST_PP_IF(align, NOTHING, U)) \
						return *this; }
			#define MUL_INNER2(z,n,AU)	_mm_add_ps(accum, _mm_mul_ps(_mm_shuffle_ps(tm,tm, _MM_SHUFFLE(n,n,n,n)), LOADPS##AU(m.ma[n])));
			#define MUL_OUTER2(z,n,AU)	{ __m128 tm = LOADTHIS(n); \
					__m128 accum = _mm_mul_ps(_mm_shuffle_ps(tm, tm, _MM_SHUFFLE(0,0,0,0)), LOADPS##AU(m.ma[0])); \
					BOOST_PP_REPEAT_FROM_TO(1,DIM_M, MUL_INNER2, AU) \
					BOOST_PP_CAT(STOREPS_, BOOST_PP_CAT(AFLAG(ALIGN),4))(ma[n], accum); }
			BOOST_PP_REPEAT(LEN_SEQ, DEF_CONV_ITR, DEF_MULE)
		}
	#else
		namespace spn {
			// 他の行列やベクトルと計算するメソッドを定義
			/*	Pseudo-code:
				template <>
				VecT<DIM_N,ALIGNB> operator * (const VecT<DIM_N,align>& v) const {
					auto tmpM = transposition();
					return v * tmpM;
				}
			*/
			#define DEF_MULOP(z,align,dummy)	template <> VecT<DIM_N,ALIGNB> MT::operator * (const VecT<DIM_N,BOOLNIZE(align)>& v) const { \
				auto tmpM = BOOST_PP_CAT(BOOST_PP_CAT(BOOST_PP_CAT(transposition().convert, AFLAG(ALIGN)), DIM_N), DIM_N)(); \
				return v * tmpM; }
			BOOST_PP_REPEAT(1, DEF_MULOP, NOTHING)
		}
	#endif
	#undef DEFINE_MATRIX
#endif
