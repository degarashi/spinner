#include "test.hpp"
#include "../resmgr.hpp"

namespace spn {
	namespace test {
		namespace {
			struct MyClass {
				int first, second;
			};
		}
		//TODO Assert文を追加
		TEST(Resource, Test) {
			struct MyDerived : MyClass {
				MyDerived(int v0, int v1) {
					first = v0;
					second = v1;
				}
			};
			struct MyMgr : ResMgrN<MyClass, MyMgr> {
				using Hdl = ResMgrN<MyClass, MyMgr>::AnotherLHandle<MyDerived, true>;
				LHdl doit() {
					auto ldhl = ResMgrN<MyClass, MyMgr>::acquire(MyClass{123,321});
					return ldhl;
				}
				Hdl doit2() {
					LHdl lh = ResMgrN<MyClass, MyMgr>::acquire(MyClass{123,321});
					return Cast<MyDerived>(std::move(lh));
				}
			};
			MyMgr mm;
			auto hdl = mm.doit();
			spn::LHandle lh;
			lh = std::move(hdl);

			int tekito = 100;
			noseq_list<int&> asd;
			asd.add(&tekito);
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
}
