/*! local_macro.hpp で定義したマクロを消去する */

#undef BASE_LOADPS
#undef LOADTHIS
#undef LOADTHISZ
#undef STORETHIS

#ifdef DEFINE_MATRIX
	#undef LOADTHISI
#endif

#undef STOREPS
#undef STOREPSU
#undef LOADTHISPS
#undef STORETHISPS

#undef LOADPS
#undef LOADPSU
#undef LOADPSZ
#undef LOADPSZU