#include <iostream>
#include "vector.hpp"
#include "matrix.hpp"
#include "matstack.hpp"
#include "pose.hpp"
#include "misc.hpp"
#include "noseq.hpp"
#include "bits.hpp"
#include "resmgr.hpp"
#include "pqueue.hpp"
#include "unittest.hpp"
#include "size.hpp"
#include <sstream>
#include <boost/serialization/access.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace spn;
namespace {
	void MatTest() {
		Mat44 m4 = Mat44::RotationZ(DEGtoRAD(0));
		auto idx = Mat44(Mat44::TagIdentity);
		auto mm4 = m4 * idx;
		m4 *= idx;

		AMat43 mrY = AMat43::RotationY(DEGtoRAD(45));
		auto mrY4 = mrY.convertA44();

		AMat33 mrX = AMat33::RotationX(DEGtoRAD(90));
		AVec3 vrot(0,0,100);
		auto vr = vrot * mrX;
		float len = vrot.length();

		using MyStack = MatStack<Mat33, MatStackTag::PushRight>;
		MyStack stk;
		stk.push(Mat33(mrX));
	}
	void PoseTest() {
		Pose2D p2(AVec2(100,200), 128, AVec2(1,1)),
		p3(AVec2(1,2), 64, AVec2(3,3));
		auto p4 = p2.lerp(p3, 0.5f);

		auto pc = AVec3::FromPacked(MakeChunk(64,128,192,255));
		auto pc2 = pc.toPacked();

		noseq_list<float> ls;
		auto id0 = ls.add(256);
		auto id1 = ls.add(512);
		auto id2 = ls.add(1024);
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
		std::cout << std::endl;
	}
	struct MyClass {
		int first, second;
		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar & first & second;
		}
	};
	void ResourceTest() {
		struct MyDerived : MyClass {
			MyDerived(int v0, int v1) {
				first = v0;
				second = v1;
			}
		};
		struct MyMgr : ResMgrN<MyClass, MyMgr> {
			using Hdl = ResMgrN<MyClass, MyMgr>::AnotherLHandle<MyDerived>;
			LHdl doit() {
				auto ldhl = ResMgrN<MyClass, MyMgr>::acquire(MyClass{123,321});
				return ldhl;
			}
		};
		MyMgr mm;
		auto hdl = mm.doit();

		int tekito = 100;
		noseq_list<int&> asd;
		auto id = asd.add(&tekito);
		int& dd = *asd.begin();
		std::cout << *asd.begin() << std::endl;
		dd = 200;
		std::cout << *asd.begin() << std::endl;
		*asd.rbegin();
		*asd.crbegin();

		for(auto itr=mm.cbegin() ; itr!=mm.cend() ; itr++) {
			std::cout << (*itr).first << std::endl;
		}

		auto hdl2 = mm.acquire("HEXEN", MyClass{256,512});
		std::stringstream ss;
		boost::archive::text_oarchive oa(ss);
		oa << mm;
		boost::archive::text_iarchive ia(ss);
		ia >> mm;
	}
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
				Assert(Trap, m.ma[i][j]==*src++)
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
			Assert(Trap, v.m[i]==*src++)
		return src;
	}
	struct Random {
		using OPMT = spn::Optional<std::mt19937>;
		using OPDistF = spn::Optional<std::uniform_real_distribution<float>>;
		using OPDistI = spn::Optional<std::uniform_int_distribution<int>>;
		OPMT	_opMt;
		OPDistF	_opDistF;
		OPDistI _opDistI;

		Random() {
			std::random_device rdev;
			std::array<uint_least32_t, 32> seed;
			std::generate(seed.begin(), seed.end(), std::ref(rdev));
			std::seed_seq seq(seed.begin(), seed.end());
			_opMt = std::mt19937(seq);
			_opDistF = std::uniform_real_distribution<float>{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()};
			_opDistI = std::uniform_int_distribution<int>{std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max()};
		}
		float randomF() {
			return (*_opDistF)(*_opMt);
		}
		int randomI() {
			return (*_opDistI)(*_opMt);
		}
		int randomI(int from, int to) {
			return std::uniform_int_distribution<int>{from, to}(*_opMt);
		}
		static Random& getInstance() {
			static Random rd;
			return rd;
		}
	};
	void GapTest() {
		auto& rd = Random::getInstance();
		auto gen = std::bind(&Random::randomF, std::ref(rd));
		std::array<float,16> tmp;
		std::generate(tmp.begin(), tmp.end(), std::ref(gen));

		#define SETAUX(z,n,data)	aux##n = *data++;
		#define CHECKAUX(z,n,data) Assert(Trap, aux##n==*data++)
		{
			GAP_MATRIX(mat, 4,3,
					(((float, aux0)))
					(((float, aux1)))
					(((float, aux2)))
					(((float, aux3)))
			)
			auto* src = SetValue(mat, &tmp[0]);
			BOOST_PP_REPEAT(4, SETAUX, src)
			src = CheckValue(mat, &tmp[0]);
			BOOST_PP_REPEAT(4, CHECKAUX, src)
		}
		{
			GAP_MATRIX(mat, 4,2,
					(((float, aux0), (float, aux4)))
					(((float, aux1), (float, aux5)))
					(((float, aux2), (float, aux6)))
					(((float, aux3), (float, aux7)))
			)
			auto* src = SetValue(mat, &tmp[0]);
			BOOST_PP_REPEAT(8, SETAUX, src)
			src = CheckValue(mat, &tmp[0]);
			BOOST_PP_REPEAT(8, CHECKAUX, src)
		}
		{
			GAP_VECTOR(vec, 3, (float aux0))
			auto* src = SetValue(vec, &tmp[0]);
			BOOST_PP_REPEAT(1, SETAUX, src)
			src = CheckValue(vec, &tmp[0]);
			BOOST_PP_REPEAT(1, CHECKAUX, src)
		}
		{
			GAP_VECTOR(vec, 2, (float aux0)(float aux1))
			auto* src = SetValue(vec, &tmp[0]);
			BOOST_PP_REPEAT(2, SETAUX, src)
			src = CheckValue(vec, &tmp[0]);
			BOOST_PP_REPEAT(2, CHECKAUX, src)
		}
		#undef SETAUX
		#undef CHECKAUX
	}
	void SerializeTest() {
		auto& rd = Random::getInstance();
		std::stringstream buffer;
		constexpr int NTEST = 256,
					NITER = 1024;
		for(int i=0 ; i<NTEST ; i++) {
			buffer.str("");

			spn::noseq_list<int> base, loaded;
			std::vector<decltype(base.add(0))> ids;
			for(int j=0 ; j<NITER ; j++) {
				// ランダムにデータを追加/削除する
				if(base.empty() || ((rd.randomI()&0x3) != 0)) {
					// 追加
					ids.push_back(base.add(rd.randomI()));
				} else {
					// 削除
					int idx = rd.randomI(0, ids.size()-1);
					auto itr = ids.begin() + idx;
					base.rem(*itr);
					ids.erase(itr);
				}
			}
			boost::archive::binary_oarchive oa(buffer);
			oa << base;
			boost::archive::binary_iarchive ia(buffer);
			ia >> loaded;
			Assert(Trap, base == loaded);
		}
	}
}
int main(int argc, char **argv) {
	MatTest();
	PoseTest();
	BitFieldTest();
	ResourceTest();
	GapTest();
	SerializeTest();
	spn::unittest::PQueue();
	spn::unittest::Math();
	return 0;
}
