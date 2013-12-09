#pragma once
#include "macro.hpp"
#include <boost/serialization/version.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/tracking.hpp>

#define ENUM_CLASSTYPE(r, data, i, elem)	(elem BOOST_PP_SEQ_ELEM(i, SEQ_ALPHABET))
#define ENUM_ALPHABET(z, n, data)			(BOOST_PP_SEQ_ELEM(n, SEQ_ALPHABET))
#define EXPAND_TEMPLATE_FORMAT(seq) \
	BOOST_PP_SEQ_ENUM( \
		BOOST_PP_SEQ_FOR_EACH_I( \
			ENUM_CLASSTYPE, \
			NOTHING, \
			seq \
		) \
	)
#define EXPAND_TEMPLATE_ARGS(seq) \
	BOOST_PP_SEQ_ENUM( \
		BOOST_PP_REPEAT( \
			BOOST_PP_SEQ_SIZE(seq), \
			ENUM_ALPHABET, \
			NOTHING \
		) \
	)

#define BOOST_CLASS_IMPLEMENTATION_TEMPLATE(seq, name, typeclass) \
	namespace boost{ namespace serialization{ \
		template <EXPAND_TEMPLATE_FORMAT(seq)> \
		struct implementation_level_impl<const name<EXPAND_TEMPLATE_ARGS(seq)>>{ \
			typedef mpl::integral_c_tag tag; \
			typedef mpl::int_<typeclass> type; \
			BOOST_STATIC_CONSTANT(int, value = implementation_level_impl::type::value); \
		}; }}
#define BOOST_CLASS_VERSION_TEMPLATE(seq, name, version_num) \
	namespace boost{ namespace serialization{ \
		template<EXPAND_TEMPLATE_FORMAT(seq)> \
		struct version<name<EXPAND_TEMPLATE_ARGS(seq)>> { \
			typedef mpl::int_<version_num> type; \
			typedef mpl::integral_c_tag tag; \
			BOOST_STATIC_CONSTANT(int, value = version::type::value); \
	}; }}
#define BOOST_CLASS_TRACKING_TEMPLATE(seq, name, typeclass) \
	namespace boost{ namespace serialization{ \
	template<EXPAND_TEMPLATE_FORMAT(seq)> \
	struct tracking_level<name<EXPAND_TEMPLATE_ARGS(seq)>> { \
		typedef mpl::integral_c_tag tag; \
		typedef mpl::int_<typeclass> type; \
		BOOST_STATIC_CONSTANT(int, value = tracking_level::type::value); \
		BOOST_STATIC_ASSERT((mpl::greater< \
				implementation_level<name<EXPAND_TEMPLATE_ARGS(seq)>>, \
				mpl::int_<primitive_type> \
			>::value \
		)); \
	}; }}
