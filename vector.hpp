//! ベクトルクラス基底ヘッダ
/*! 2-4次元のベクトルをアラインメント有りと無しでそれぞれ定義
	初期化や要素毎のアクセスなど基本的なメソッド */
#if !BOOST_PP_IS_ITERATING
	#if !defined(VECTOR_H_) || INCLUDE_VECTOR_LEVEL >= 1
		#define VECTOR_H_
		#include <boost/preprocessor.hpp>
		#include <boost/operators.hpp>
		#include <boost/serialization/access.hpp>
		#include <boost/serialization/level.hpp>
		#include <boost/serialization/nvp.hpp>
		#include <cmath>
		#include "spn_math.hpp"
		#include "tostring.hpp"
		#include "random/vector.hpp"

		#define DEF_ARGPAIR(index,data,elem)	(float BOOST_PP_CAT(f, elem))
		#define ENUM_ARGPAIR(subseq) BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(DEF_ARGPAIR, NOTHING, subseq))
		#define DEF_ARGSET(index,data,elem)		elem = BOOST_PP_CAT(f, elem);
		#define SEQ_VECELEM (x)(y)(z)(w)

		// 要求された定義レベルを実体化
		#ifndef INCLUDE_VECTOR_LEVEL
			#define INCLUDE_VECTOR_LEVEL 0
		#endif
		#define BOOST_PP_ITERATION_PARAMS_1 (4, (2,4, "vector.hpp", INCLUDE_VECTOR_LEVEL))
		#include BOOST_PP_ITERATE()
		#undef INCLUDE_VECTOR_LEVEL
	#endif
#elif BOOST_PP_ITERATION_DEPTH() == 1
	// アラインメントイテレーション
	#define DIM		BOOST_PP_FRAME_ITERATION(1)
	#define BOOST_PP_ITERATION_PARAMS_2 (4, (0,1, "vector.hpp", BOOST_PP_FRAME_FLAGS(1)))
	#include BOOST_PP_ITERATE()
	#undef DIM
#else
	#define ALIGN	BOOST_PP_ITERATION()
	#define ALIGNB	BOOLNIZE(ALIGN)
	#define Vec		BOOST_PP_IF(ALIGN, alignas(16), NOTHING) VecT<DIM, ALIGNB>
	#define VT		VecT<DIM, ALIGNB>
	#include "local_macro.hpp"
	#if BOOST_PP_ITERATION_FLAGS() == 0
		// クラスのコンストラクタとメソッドのプロトタイプだけ定義
		namespace spn {
			class MTRandom;
			template <>
			struct Vec {
				constexpr static int width = DIM;
				constexpr static bool align = ALIGNB;
				using AVec = VecT<DIM,true>;
				using UVec = VecT<DIM,false>;
				union {
					struct {
						float BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, DIM));
					};
					float m[DIM];
				};

				friend class boost::serialization::access;
				template <class Archive>
				void serialize(Archive& ar, const unsigned int /*ver*/) {
					ar & BOOST_SERIALIZATION_NVP(m);
				}

				// -------------------- ctor --------------------
				VecT() = default;
				explicit VecT(reg128 r);
				explicit VecT(float a);
				explicit VecT(ENUM_ARGPAIR(BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, DIM)));
				template <class V, class=std::enable_if_t<std::is_floating_point<V>::value>>
				explicit VecT(const V* src);
				VecT(std::initializer_list<float> il);
				reg128 loadPS() const;
				reg128 loadPSZ() const;

				//! アラインメント済ベクトルで初期化
				VecT(const AVec& v);
				//! アラインメント無しベクトルで初期化
				VecT(const UVec& v);
				VecT& operator = (const AVec& v);
				VecT& operator = (const UVec& v);
				VecT& operator = (reg128 r);
				VecT& operator = (std::initializer_list<float> il);
				// -------------------- operators --------------------
				// ベクトルとの積算や除算は同じ要素同士の演算とする
				#define DEF_PRE(op, func)	VecT& operator BOOST_PP_CAT(op,=) (float s); \
					VecT operator op (float s) const; \
					VecT& operator BOOST_PP_CAT(op,=) (const AVec& v); \
					VecT& operator BOOST_PP_CAT(op,=) (const UVec& v); \
					AVec operator op (const AVec& v) const; \
					UVec operator op (const UVec& v) const; \
					AVec&& operator op (AVec&& v) const; \
					UVec&& operator op (UVec&& v) const; \
					friend VecT operator op (float fv, const VecT& v);
				DEF_PRE(+, reg_add_ps)
				DEF_PRE(-, reg_sub_ps)
				DEF_PRE(*, reg_mul_ps)
				DEF_PRE(/, _mmDivPs)

				// -------- Luaへのエクスポート用 --------
				VecT addV(const UVec& v) const;
				VecT subV(const UVec& v) const;
				VecT mulF(float s) const;
				VecT mulM(const MatT<DIM,DIM,align>& m) const;
				VecT divF(float s) const;
				VecT invert() const;
				bool equal(const UVec& v) const;
				std::string toString() const;

				friend std::ostream& operator << (std::ostream& os, const VT& v);
				// -------------------- others --------------------
				static float _sumup(reg128 xm);
				// ロード関数呼び出しのコストが許容出来るケースではloadPS()を呼び、そうでないケースはオーバーロードで対処
				template <bool A>
				float dot(const VecT<DIM,A>& v) const;
				//! 各要素を足しあわせた合計
				float sum() const;
				//! 各要素の算術平均
				float average() const;
				template <bool A>
				float distance(const VecT<DIM,A>& v) const;
				template <bool A>
				float dist_sq(const VecT<DIM,A>& v) const;

				//! 要素ごとに最小値選択
				template <bool A>
				VecT getMin(const VecT<DIM,A>& v) const;
				template <bool A>
				void selectMin(const VecT<DIM,A>& v);

				//! 要素ごとに最大値選択
				template <bool A>
				VecT getMax(const VecT<DIM,A>& v) const;
				template <bool A>
				void selectMax(const VecT<DIM,A>& v);

				VecT operator - () const;

				/*! \return 要素が全て等しい時にtrue, それ以外はfalse */
				template <bool A>
				bool operator == (const VecT<DIM,A>& v) const;
				template <bool A>
				bool operator != (const VecT<DIM,A>& v) const;

				float normalize();
				VecT normalization() const;
				/*! \return ベクトルの長さ */
				float length() const;
				/*! \return ベクトル長の2乗 */
				float len_sq() const;
				//! 要素の何れかがNaNになっているか
				bool isNaN() const;
				//! 要素の何れかがNaN又はinfになっているか
				bool isOutstanding() const;

				void saturate(float fMin, float fMax);
				VecT saturation(float fMin, float fMax) const;
				void lerp(const UVec& v, float r);
				template <bool A>
				VecT l_intp(const VecT<DIM,A>& v, float r) const;

				//! 要素の値がランダムなベクトル
				template <class RDF>
				static VecT Random(const RDF& rdf, const RangeF& r=random::DefaultRVecRange) {
					return random::GenRVec<VecT>(rdf, r);
				}
				template <class RDF>
				static VecT RandomF(const RDF& rdf) {
					return random::GenRVecF<VecT>(rdf);
				}
				//! ランダムなベクトル（但し長さが指定値より大きい）
				template <class RDF>
				static VecT RandomWithLength(const RDF& rdf, float th, const RangeF& r=random::DefaultRVecRange) {
					return random::GenRVecLen<VecT>(rdf, th, r);
				}
				//! ランダムなベクトル (但し全ての成分の絶対値がそれぞれ基準範囲内)
				template <class RDF>
				static VecT RandomWithAbs(const RDF& rdf, const RangeF& rTh, const RangeF& r=random::DefaultRVecRange) {
					return random::GenRVecAbs<VecT>(rdf, rTh, r);
				}
				//! ランダムな方向ベクトル
				template <class RDF>
				static VecT RandomDir(const RDF& rdf) {
					return random::GenRDir<VecT>(rdf);
				}
				// -------- Luaへのエクスポート用 --------
				static VecT luaRandom(MTRandom& mt, const RangeF& r);
				static VecT luaRandomWithLength(MTRandom& mt, float th, const RangeF& r);
				static VecT luaRandomWithAbs(MTRandom& mt, const RangeF& rTh, const RangeF& r);
				#if ALIGN==1
					//! AVec -> Vec へ暗黙変換
					operator VecT<DIM,false>& ();
					operator const VecT<DIM,false>& () const;
				#endif
				#if DIM==4
					using Vec3 = VecT<3,ALIGNB>;
					/*! \return x,y,z成分 */
					const Vec3& asVec3() const;
					/*! \return x,y,zそれぞれをwで割った値 */
					Vec3 asVec3Coord() const;
				#elif DIM==3
					//! ベクトルに垂直なベクトルを適当に定める
					VecT verticalVector() const;
					/*! \return this X v の外積ベクトル */
					template <bool A>
					VecT cross(const VecT<DIM,A>& v) const;
					//! 外積計算cross()と同義
					template <bool A>
					void operator %= (const VecT<DIM,A>& v);
					//! 外積計算cross()と同義
					template <bool A>
					VecT operator % (const VecT<DIM,A>& v) const;
					using Vec4 = VecT<4,ALIGNB>;
					Vec4 asVec4(float w) const;
					//! 平面との交差点を算出
					template <bool A>
					std::tuple<VecT,bool> planeDivide(const VecT<DIM,A>& v, const PlaneT<false>& p) const;
					//! 平面にてベクトルを反転
					void flip(const PlaneT<false>& plane);
					VecT& operator *= (const QuatT<ALIGNB>& q);
					VecT operator * (const QuatT<ALIGNB>& q) const;
					VecT&& operator * (QuatT<ALIGNB>&& q) const;
					/*! \return x,y 成分 */
					const VecT<2,ALIGNB>& asVec2() const;
					// -------- Luaへのエクスポート用 --------
					VecT modV(const UVec& v) const;
					VecT mulQ(const QuatT<ALIGNB>& q) const;
					std::tuple<VecT,bool> luaPlaneDivide(const VecT<DIM,false>& v, const PlaneT<false>& p) const;
				#elif DIM==2
					float ccw(const VecT<DIM,false>& v) const;
					float cw(const VecT<DIM,false>& v) const;
					#if ALIGN==0
						static float Ccw(const UVec& v0, const UVec& v1, const UVec& v2);
						static float Cw(const UVec& v0, const UVec& v1, const UVec& v2);
					#endif
					using Vec3 = VecT<3,ALIGNB>;
					Vec3 asVec3(float z) const;
				#endif
				//! 行列との積算 (左から掛ける)
				/*! 行ベクトルとして扱う */
				template <int N, bool A>
				VecT<N,ALIGNB> operator * (const MatT<DIM,N,A>& m) const;
				template <int N, bool A>
				VecT& operator *= (const MatT<DIM,N,A>& m);

				#if DIM>=3
					//! パックされた32bit数値から色を取り出す
					static VecT FromPacked(uint32_t val);
					//! 32bit数値に値をパック (1.0が255)
					uint32_t toPacked() const;
				#endif
			};
			#undef Vec
			// 使いやすいようにクラスの別名を定義
			using BOOST_PP_CAT(BOOST_PP_IF(ALIGN,A,NOTHING), BOOST_PP_CAT(Vec,DIM)) = VT;
		}
		BOOST_CLASS_IMPLEMENTATION(spn::VT, object_serializable)
		#elif BOOST_PP_ITERATION_FLAGS() == 1
		// 同次元ベクトルとの演算を定義
		namespace spn {
			VT::VecT(reg128 r){
				STORETHIS(r);
			}
			VT::VecT(float a) {
				STORETHIS(reg_load1_ps(&a));
			}
			VT::VecT(ENUM_ARGPAIR(BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, DIM))) {
				BOOST_PP_SEQ_FOR_EACH(
					DEF_ARGSET,
					NOTHING,
					BOOST_PP_SEQ_SUBSEQ(SEQ_VECELEM, 0, DIM)
				)
			}
			template <class V, class>
			VT::VecT(const V* src) {
				for(int i=0 ; i<DIM ; i++)
					m[i] = src[i];
			}
			template VT::VecT(const float*);
			template VT::VecT(const double*);
			VT::VecT(std::initializer_list<float> il) {
				// 要素数が満たなかったら残りは0にする
				alignas(16) float tmp[4] = {};
				auto* pTmp = tmp;
				for(auto itr=il.begin() ; itr!=il.end() ; ++itr)
					*pTmp++ = *itr;
				STORETHIS(reg_load_ps(tmp));
			}
			reg128 VT::loadPS() const {
				return LOADTHIS();
			}
			reg128 VT::loadPSZ() const {
				return LOADTHISZ();
			}
			VT& VT::operator = (reg128 r) {
				STORETHIS(r);
				return *this;
			}
			VT& VT::operator = (std::initializer_list<float> il) {
				return *this = VecT(il);
			}
			float VT::_sumup(reg128 xm) {
				SUMVEC(xm)
				float ret;
				reg_store_ss(&ret, xm);
				return ret;
			}
			VT::VecT(const AVec& v) {
				STORETHIS(LOADPS(v.m)); }
			VT::VecT(const UVec& v) {
				STORETHIS(LOADPSU(v.m)); }
			bool VT::isNaN() const {
				reg128 r0 = LOADTHISZ(),
					r_zero = reg_setzero_ps();
				reg128 res = reg_or_ps(reg_cmple_ps(r0, r_zero),
										reg_cmpgt_ps(r0, r_zero));
				res = reg_andnot_ps(res, xmm_const::fullbit());
				SUMVEC(res)
				float f;
				reg_store_ss(&f, res);
				return f != 0;
			}
			bool VT::isOutstanding() const {
				const float f = std::numeric_limits<float>::infinity();
				reg128 r_inf = reg_load1_ps(&f),
						r_zero = reg_setzero_ps(),
						r0 = reg_and_ps(LOADTHIS(), xmm_const::absmask());
				reg128 r1 = reg_or_ps(reg_cmple_ps(r0, r_zero),
									reg_cmpgt_ps(r0, r_zero));
				r0 = reg_cmpeq_ps(r0, r_inf);

				r1 = reg_andnot_ps(r1, xmm_const::fullbit());
				r0 = reg_or_ps(r0, r1);
				SUMVEC(r0)

				float fv;
				reg_store_ss(&fv, r0);
				return fv != 0;
			}

			VT& VT::operator = (const AVec& v) {
				STORETHIS(LOADPS(v.m));
				return *this;
			}
			VT& VT::operator = (const UVec& v) {
				STORETHIS(LOADPSU(v.m));
				return *this;
			}
			#define DEF_OP(op, func) \
					VT& VT::operator BOOST_PP_CAT(op,=) (float s) { \
						STORETHIS(func(LOADTHIS(), reg_load_ps1(&s))); \
						return *this; } \
					VT VT::operator op (float s) const { \
						return VecT(func(LOADTHIS(), reg_load1_ps(&s))); } \
					VT& VT::operator BOOST_PP_CAT(op,=) (const AVec& v) { \
						STORETHIS(func(LOADTHIS(), LOADPS(v.m))); \
						return *this; } \
					VT& VT::operator BOOST_PP_CAT(op,=) (const UVec& v) { \
						STORETHIS(func(LOADTHIS(), LOADPSU(v.m))); \
						return *this; } \
					VT::AVec VT::operator op (const AVec& v) const { \
						return AVec(func(LOADTHIS(), LOADPS(v.m))); } \
					VT::UVec VT::operator op (const UVec& v) const { \
						return UVec(func(LOADTHIS(), LOADPSU(v.m))); } \
					VT::AVec&& VT::operator op (AVec&& v) const { \
						STOREPS(v.m, func(LOADTHIS(), LOADPS(v.m))); \
						return std::forward<AVec>(v); } \
					VT::UVec&& VT::operator op (UVec&& v) const { \
						STOREPSU(v.m, func(LOADTHIS(), LOADPSU(v.m))); \
						return std::forward<UVec>(v); } \
					VT operator op (float fv, const VT& v) { \
						return VT(func(LOADTHISPS(v.m), reg_load1_ps(&fv))); }
			DEF_OP(+, reg_add_ps)
			DEF_OP(-, reg_sub_ps)
			DEF_OP(*, reg_mul_ps)
			DEF_OP(/, _mmDivPs)
			#undef DEF_OP
			VT VT::addV(const UVec& v) const {
				return *this + v; }
			VT VT::subV(const UVec& v) const {
				return *this - v; }
			VT VT::mulF(float s) const {
				return *this * s; }
			VT VT::mulM(const MatT<DIM,DIM,align>& m) const {
				return *this * m; }
			VT VT::divF(float s) const {
				return *this / s; }
			VT VT::invert() const {
				return -(*this); }
			bool VT::equal(const UVec& v) const {
				return *this == v; }
			std::string VT::toString() const {
				return ToString(*this);
			}

			std::ostream& operator << (std::ostream& os, const VT& v) {
				os << BOOST_PP_IF(ALIGN, 'A', ' ') << "Vec" << DIM << '[';
				for(int i=0 ; i<DIM-1 ; i++)
					os << v.m[i] << ',';
				return os << v.m[DIM-1] << ']';
			}

			template <bool A>
			float VT::dot(const VecT<DIM,A>& v) const {
				return _sumup(reg_mul_ps(LOADTHISZ(), v.loadPSZ()));
			}
			template float VT::dot(const VecT<DIM,false>& v) const;
			template float VT::dot(const VecT<DIM,true>& v) const;

			float VT::sum() const {
				return _sumup(LOADTHISZ());
			}
			float VT::average() const {
				return sum() * spn::Rcp22Bit(DIM);
			}
			template <bool A>
			float VT::distance(const VecT<DIM,A>& v) const {
				return std::sqrt(dist_sq(v));
			}
			template float VT::distance(const VecT<DIM,false>&) const;
			template float VT::distance(const VecT<DIM,true>&) const;

			template <bool A>
			float VT::dist_sq(const VecT<DIM,A>& v) const {
				auto tv = v - *this;
				return tv.len_sq();
			}
			template float VT::dist_sq(const VecT<DIM,false>&) const;
			template float VT::dist_sq(const VecT<DIM,true>&) const;

			template <bool A>
			VT VT::getMin(const VecT<DIM,A>& v) const {
				return VT(reg_min_ps(LOADTHIS(), v.loadPS())); }
			template VT VT::getMin(const VecT<DIM,false>&) const;
			template VT VT::getMin(const VecT<DIM,true>&) const;

			template <bool A>
			void VT::selectMin(const VecT<DIM,A>& v) {
				STORETHIS(reg_min_ps(LOADTHIS(), v.loadPS())); }
			template void VT::selectMin(const VecT<DIM,false>&);
			template void VT::selectMin(const VecT<DIM,true>&);

			template <bool A>
			VT VT::getMax(const VecT<DIM,A>& v) const {
				return VT(reg_max_ps(LOADTHIS(), v.loadPS())); }
			template VT VT::getMax(const VecT<DIM,false>&) const;
			template VT VT::getMax(const VecT<DIM,true>&) const;

			template <bool A>
			void VT::selectMax(const VecT<DIM,A>& v) {
				STORETHIS(reg_max_ps(LOADTHIS(), v.loadPS())); }
			template void VT::selectMax(const VecT<DIM,false>&);
			template void VT::selectMax(const VecT<DIM,true>&);

			VT VT::operator - () const {
				return *this * -1.0f;
			}

			template <bool A>
			bool VT::operator == (const VecT<DIM,A>& v) const {
				reg128 r0 = reg_cmpeq_ps(LOADTHISZ(), v.loadPSZ());
				r0 = reg_and_ps(r0, reg_shuffle_ps(r0, r0, _REG_SHUFFLE(1,0,3,2)));
				r0 = reg_and_ps(r0, reg_shuffle_ps(r0, r0, _REG_SHUFFLE(0,1,2,3)));
				return reg_cvttss_si32(r0) != 0;
			}
			template <bool A>
			bool VT::operator != (const VecT<DIM,A>& v) const {
				return !(this->operator == (v));
			}
			template bool VT::operator == (const VecT<DIM,false>&) const;
			template bool VT::operator == (const VecT<DIM,true>&) const;
			template bool VT::operator != (const VecT<DIM,false>&) const;
			template bool VT::operator != (const VecT<DIM,true>&) const;

			float VT::normalize() {
				float len = length();
				*this *= spn::Rcp22Bit(len);
				return len;
			}
			VT VT::normalization() const {
				float tmp = length();
				reg128 r0 = reg_load_ps1(&tmp);
				r0 = reg_div_ps(LOADTHIS(), r0);
				return VT(r0);
			}
			/*! \return ベクトルの長さ */
			float VT::length() const {
				return std::sqrt(len_sq());
			}
			/*! \return ベクトル長の2乗 */
			float VT::len_sq() const {
				reg128 r0 = LOADTHISZ();
				r0 = reg_mul_ps(r0, r0);
				SUMVEC(r0)

				float ret;
				reg_store_ss(&ret, r0);
				return ret;
			}
			void VT::saturate(float fMin, float fMax) {
				*this = saturation(fMin, fMax);
			}
			VT VT::saturation(float fMin, float fMax) const {
				reg128 xm = reg_max_ps(LOADTHIS(), reg_load1_ps(&fMin));
				return VT(reg_min_ps(xm, reg_load1_ps(&fMax)));
			}
			void VT::lerp(const UVec& v, float r) {
				*this = l_intp(v,r);
			}
			template <bool A>
			VT VT::l_intp(const VecT<DIM,A>& v, float r) const {
				reg128 ths = LOADTHIS();
				ths = reg_add_ps(ths, reg_mul_ps(reg_sub_ps(v.loadPS(), ths), reg_load1_ps(&r)));
				return VT(ths);
			}
			template VT VT::l_intp(const VecT<DIM,false>&, float) const;
			template VT VT::l_intp(const VecT<DIM,true>&, float) const;

			#if DIM>=3
				VT VT::FromPacked(uint32_t val) {
					alignas(16) const uint32_t tmp[4] = {val, val>>8, val>>16, val>>24};
					alignas(16) const uint32_t mask[4] = {0xff,0xff,0xff,0xff};

					reg128 x = reg_and_ps(reg_load_ps((const float*)mask), reg_load_ps((const float*)tmp));
					x = reg_cvtepi32_ps((reg128i)x);

					const float f255 = 1.f/255;
					x = reg_mul_ps(x, reg_load1_ps(&f255));
					return VecT(x);
				}
				uint32_t VT::toPacked() const {
					reg128 x = reg_mul_ps(reg_set_ps(255,255,255,255), LOADTHISZ());
					reg128i x2 = reg_cvtps_epi32(x);
					x2 = reg_packs_epi32(x2,x2);
					x2 = reg_packus_epi16(x2,x2);
					float ret;
					reg_store_ss(&ret, (reg128)x2);
					return *reinterpret_cast<uint32_t*>(&ret);
				}
			#endif
		}
	#elif BOOST_PP_ITERATION_FLAGS() == 2
		// 他次元ベクトルとの演算を定義
		namespace spn {
			#if ALIGN==1
				//! AVec -> Vec へ暗黙変換
				VT::operator VecT<DIM,false>& () { return *reinterpret_cast<VecT<DIM,false>*>(this); }
				VT::operator const VecT<DIM,false>& () const { return *reinterpret_cast<const VecT<DIM,false>*>(this); }
			#endif
			#if DIM==4
				#define _Vec3 VecT<3,ALIGNB>
				/*! \return x,y,z成分 */
				const _Vec3& VT::asVec3() const {
					return reinterpret_cast<const _Vec3&>(*this);
				}
				/*! \return x,y,zそれぞれをwで割った値 */
				_Vec3 VT::asVec3Coord() const {
					reg128 r0 = reg_rcp_ps(reg_load_ps1(&w));
					return _Vec3(reg_mul_ps(LOADTHIS(), r0));
				}
			#elif DIM==3
				const VecT<2,ALIGNB>& VT::asVec2() const {
					return reinterpret_cast<const VecT<2,ALIGNB>&>(*this);
				}
				//! ベクトルに垂直なベクトルを適当に定める
				VT VT::verticalVector() const {
					VT ret = VT(1,0,0) % *this;
					float len_s = ret.len_sq();
					if(len_s < 1e-6f) {
						ret = VT(0,0,1) % *this;
						return ret.normalization();
					}
					return ret * RSqrt(len_s);
				}
				/*! \return this X v の外積ベクトル */
				template <bool A>
				VT VT::cross(const VecT<DIM,A>& v) const {
					reg128 r0 = LOADTHIS(),
							r1 = v.loadPS();
					// r0[y,z,x]
					reg128 m0 = reg_shuffle_ps(r0, r0, _REG_SHUFFLE(0,0,2,1)),
					// r0[z,x,y]
							m1 = reg_shuffle_ps(r0, r0, _REG_SHUFFLE(0,1,0,2)),
					// r1[z,x,y]
							m2 = reg_shuffle_ps(r1, r1, _REG_SHUFFLE(0,1,0,2)),
					// r1[y,z,x]
							m3 = reg_shuffle_ps(r1, r1, _REG_SHUFFLE(0,0,2,1));
					r0 = reg_mul_ps(m0,m2);
					r1 = reg_mul_ps(m1,m3);
					r0 = reg_sub_ps(r0, r1);
					return VT(r0);
				}
				template VT VT::cross(const VecT<DIM,false>&) const;
				template VT VT::cross(const VecT<DIM,true>&) const;

				//! 外積計算cross()と同義
				template <bool A>
				void VT::operator %= (const VecT<DIM,A>& v) {
					*this = cross(v);
				}
				template void VT::operator %= (const VecT<DIM,false>&);
				template void VT::operator %= (const VecT<DIM,true>&);

				//! 外積計算cross()と同義
				template <bool A>
				VT VT::operator % (const VecT<DIM,A>& v) const {
					return cross(v);
				}
				template VT VT::operator % (const VecT<DIM,false>&) const;
				template VT VT::operator % (const VecT<DIM,true>&) const;

				VT VT::modV(const UVec& v) const {
					return *this % v; }
				VT VT::mulQ(const QuatT<ALIGNB>& q) const {
					return *this * q; }
				std::tuple<VT,bool> VT::luaPlaneDivide(const VecT<DIM,false>& v, const PlaneT<false>& p) const {
					return planeDivide(v, p);
				}
				#define _Vec4 VecT<4,ALIGNB>
				_Vec4 VT::asVec4(float w) const {
					return _Vec4(x,y,z,w);
				}
			#elif DIM==2
				//! counter clockwise
				/*! 半時計回りが正数を返す */
				float VT::ccw(const UVec& v) const {
					return x*v.y - y*v.x;
				}
				//! clockwise
				/*! 時計回りが正数を返す */
				float VT::cw(const UVec& v) const {
					return -x*v.y + y*v.x;
				}
				#define _Vec3 VecT<3,ALIGNB>
				_Vec3 VT::asVec3(float z) const {
					return _Vec3(x,y,z);
				}
				#if ALIGN==0
					float VT::Ccw(const UVec& v0, const UVec& v1, const UVec& v2) {
						return (v0-v1).ccw(v2-v1);
					}
					float VT::Cw(const UVec& v0, const UVec& v1, const UVec& v2) {
						return (v0-v1).cw(v2-v1);
					}
				#endif
			#endif
		}
	#else
		#include "random.hpp"
		namespace spn {
			/*	Pseudo-code:
				template <int N, bool A>
				VecT<N,ALIGNB> VT::operator * (const MatT<DIM,N,A>& m) const {
					reg128 ths = LOADTHIS(),
							accum = reg_setzero_ps();
					for(int i=0 ; i<DIM ; i++) {
						reg128 tmp = reg_shuffle_ps(ths, ths, _REG_SHUFFLE(i,i,i,i));
						accum = reg_add_ps(accum, reg_mul_ps(tmp, LOADPS_(N)(m.ma[i])));
					}
					return VecT<N,ALIGNB>(accum);
				}
			*/
			#define LOOP_MULOP(z,n,loadf)		{reg128 tmp=reg_shuffle_ps(ths,ths, _REG_SHUFFLE(n,n,n,n)); \
						accum = reg_add_ps(accum, reg_mul_ps(tmp, loadf(mat.ma[n])));}
			#define DEF_MULOPA(z,n,align)	template <> VecT<n,ALIGNB> VT::operator * (const MatT<DIM,n,BOOLNIZE(align)>& mat) const { \
						reg128 ths = LOADTHIS(), \
						accum = reg_setzero_ps(); \
						BOOST_PP_REPEAT_##z(DIM, LOOP_MULOP, BOOST_PP_CAT(LOADPS_, BOOST_PP_CAT(AFLAG(align),n))) \
						return VecT<n,ALIGNB>(accum); }
			#define DEF_MULOPA_2(z,n,align)	template <> VT& VT::operator *= (const MatT<DIM,DIM,BOOLNIZE(align)>& mat) { \
					return *this = *this * mat; }

			#define DEF_MULOP(z,align,dummy)	BOOST_PP_REPEAT_FROM_TO_##z(2,5, DEF_MULOPA, align) DEF_MULOPA_2(dummy,dummy,align)
			BOOST_PP_REPEAT(2, DEF_MULOP, NOTHING)
			#undef DEF_MULOP

			#if DIM==3
				VT& VT::operator *= (const QuatT<ALIGNB>& q) {
					*this = (*this * q);
					return *this;
				}
				VT VT::operator * (const QuatT<ALIGNB>& q) const {
					auto tq = q * QuatT<ALIGNB>(*this,0) * q.conjugation();
					return tq.getVector();
				}
				VT&& VT::operator * (QuatT<ALIGNB>&& q) const {
					VT& rVT = *reinterpret_cast<VT*>(&q);
					rVT = *this * q;
					return std::move(rVT);
				}
				void VT::flip(const Plane& plane) {
					const auto& nml = plane.getNormal();
					float d = plane.dot(*this);
					*this += nml*-d*2;
				}
				template <bool A>
				std::tuple<VT,bool> VT::planeDivide(const VecT<DIM,A>& v, const PlaneT<false>& p) const {
					// 線分が平面をまたぐか
					float distf = p.dot(*this);
					float distb = p.dot(v);
					if(distf * distb >= 0.0f)
						return std::make_tuple(VecT(),false);

					float ratio = fabs(distf) / (fabs(distf) + fabs(distb));
					// 平面と線分の交点 -> tv
					Vec3 tv = v - (*this);
					tv *= ratio;
					VecT cp = tv + (*this);
					return std::make_tuple(cp,true);
				}
				template std::tuple<VT,bool> VT::planeDivide(const VecT<DIM,true>&,const PlaneT<false>&) const;
				template std::tuple<VT,bool> VT::planeDivide(const VecT<DIM,false>&,const PlaneT<false>&) const;
			#endif

			VT VT::luaRandom(MTRandom& mt, const RangeF& r) {
				return Random(mt.getUniformF<float>(), r);
			}
			VT VT::luaRandomWithLength(MTRandom& mt, float th, const RangeF& r) {
				return RandomWithLength(mt.getUniformF<float>(), th, r);
			}
			VT VT::luaRandomWithAbs(MTRandom& mt, const RangeF& rTh, const RangeF& r) {
				return RandomWithAbs(mt.getUniformF<float>(), rTh, r);
			}
		}
	#endif
	#include "local_unmacro.hpp"
	#undef ALIGN
	#undef ALIGNB
	#undef Vec
	#undef VT
#endif
