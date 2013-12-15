#include "resource.hpp"
#include "../resmgr.hpp"

namespace spn {
	namespace test {
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
		}
	}
}
