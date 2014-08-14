#pragma once
#include "serialization/traits.hpp"
#include <memory>
#include <boost/serialization/split_free.hpp>
namespace boost {
	namespace serialization {
		template <class Archive, class T, class D>
		void serialize(Archive& ar, std::unique_ptr<T,D>& p, const unsigned int ver) {
			split_free(ar, p, ver);
		}
		template <class Archive, class T, class D>
		void load(Archive& ar, std::unique_ptr<T,D>& p, const unsigned int) {
			T* ptr;
			ar & BOOST_SERIALIZATION_NVP(ptr);
			p.reset(ptr);
		}
		template <class Archive, class T, class D>
		void save(Archive& ar, const std::unique_ptr<T,D>& p, const unsigned int) {
			auto* ptr = p.get();
			ar & BOOST_SERIALIZATION_NVP(ptr);
		}

		template <class Archive, class T>
		void serialize(Archive& ar, std::shared_ptr<T>& p, const unsigned int ver) {
			split_free(ar, p, ver);
		}
		template <class Archive, class T>
		void load(Archive& ar, std::shared_ptr<T>& p, const unsigned int) {
			T* ptr;
			ar & BOOST_SERIALIZATION_NVP(ptr);
			p = ptr;
		}
		template <class Archive, class T>
		void save(Archive& ar, const std::shared_ptr<T>& p, const unsigned int) {
			auto* ptr = p.get();
			ar & BOOST_SERIALIZATION_NVP(ptr);
		}
	}
}
BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class)(class), std::unique_ptr, object_serializable)
BOOST_CLASS_TRACKING_TEMPLATE((class)(class), std::unique_ptr, track_never)
BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class), std::shared_ptr, object_serializable)
BOOST_CLASS_TRACKING_TEMPLATE((class), std::shared_ptr, track_never)
