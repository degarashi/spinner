#include <iostream>
#include "vector.hpp"
#include "matrix.hpp"
#include "matstack.hpp"
#include "pose.hpp"
#include "misc.hpp"
#include "noseq.hpp"
#include "bits.hpp"
#include "resmgr.hpp"

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
	void ResourceTest() {
		struct MyClass {
			int first, second;
		};
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
	}
}
int main(int argc, char **argv) {
	MatTest();
	PoseTest();
	BitFieldTest();
	ResourceTest();
	return 0;
}
