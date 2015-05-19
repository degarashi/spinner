#pragma once
#include <boost/preprocessor.hpp>

#define DU_ENUMTYPE(z, _, elem) (BOOST_PP_TUPLE_ELEM(0,elem))
#define DU_ENUMDEFINE(z, _, elem) BOOST_PP_TUPLE_ELEM(0,elem) BOOST_PP_TUPLE_ELEM(1,elem);
#define DefUnionTuple(name, seq) \
	union name { \
		using Tuple = std::tuple<BOOST_PP_SEQ_ENUM( \
							BOOST_PP_SEQ_FOR_EACH( \
								DU_ENUMTYPE, \
								BOOST_PP_NIL, \
								seq \
							) \
						)>; \
		Tuple	tuple; \
		struct { \
			BOOST_PP_SEQ_FOR_EACH(DU_ENUMDEFINE, BOOST_PP_NIL, BOOST_PP_SEQ_REVERSE(seq)) \
		}; \
		name(): tuple() {} \
		decltype(auto) getTuple() { return tuple; } \
		decltype(auto) getTuple() const { return tuple; } \
		name& operator = (const name& n) { tuple = n.tuple; return *this; } \
	};

