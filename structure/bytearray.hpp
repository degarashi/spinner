#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/level.hpp>

namespace spn {
	//! std::stringと相互に変換可能なバイト配列
	class ByteArray : public std::vector<uint8_t> {
		using base_t = std::vector<uint8_t>;
		friend class boost::serialization::access;
		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(base_t);
		}
		public:
			using base_t::base_t;
			ByteArray& operator = (const std::string& s) {
				auto len = s.length();
				resize(len);
				std::memcpy(data(), s.data(), len);
				return *this;
			}
			std::string toString() const {
				std::string str;
				auto sz = size();
				str.resize(sz);
				std::memcpy(&str[0], data(), sz);
				return std::move(str);
			}
	};
}
BOOST_CLASS_IMPLEMENTATION(spn::ByteArray, object_serializable)

