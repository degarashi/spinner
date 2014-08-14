#pragma once
#include "serialization/traits.hpp"
#include <boost/serialization/nvp.hpp>

namespace boost {
	namespace serialization {
		template <class Archive, class T0, class T1>
		void serialize(Archive& ar, std::pair<T0,T1>& p, const unsigned int) {
			using boost::serialization::make_nvp;
			ar & make_nvp("first", p.first) & make_nvp("second", p.second);
		}
	}
}
BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class)(class), std::pair, object_serializable)

