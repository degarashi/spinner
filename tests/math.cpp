#include "math.hpp"
#include "../vector.hpp"
#include "../matrix.hpp"
#include "../matstack.hpp"
#include "../pose.hpp"
#include "../bits.hpp"
#include "../noseq.hpp"
#include "test.hpp"

namespace spn {
	namespace test {
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
	}
}
