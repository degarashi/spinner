#pragma once
#define BOOST_PP_VARIADICS 1
#include <boost/serialization/level.hpp>
#include <cstdint>

BOOST_CLASS_IMPLEMENTATION(char16_t, object_serializable)
BOOST_CLASS_IMPLEMENTATION(char32_t, object_serializable)
namespace boost {
    namespace serialization {
        template <class Archive>
        void serialize(Archive& ar, char16_t& ch, const unsigned int) {
            ar & reinterpret_cast<uint16_t&>(ch);
        }
        template <class Archive>
        void serialize(Archive& ar, char32_t& ch, const unsigned int) {
            ar & reinterpret_cast<uint32_t&>(ch);
        }
    }
}
