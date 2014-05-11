cmake_minimum_required(VERSION 2.8)
project(spinner)

# Linux: BUILD_TYPE = Linux
# MinGW: BUILD_TYPE = MinGW
# Android:	BUILD_TYPE = AndroidX86 or AndroidArm
message(STATUS ${BUILD_TYPE})
include("${BUILD_TYPE}.cmake" require)

# OS依存ファイルの設定
if(${WIN32})
	set(CXX_SRC ${CXX_SRC} dir_depWin.cpp watch_depWin.cpp)
elseif(${UNIX})
	set(CXX_SRC ${CXX_SRC} dir_depLinux.cpp watch_depLinux.cpp)
	set(LINK_LIBS boost_thread boost_system pthread)
else()
	message(FATAL_ERROR "couldn't detect OS type")
endif()

set(CXX_SRC ${CXX_SRC} filetree.cpp error.cpp adaptstd.cpp ziptree.cpp zip.cpp spn_math.cpp spn_math2.cpp spn_math3.cpp spn_math4.cpp pathblock.cpp watch.cpp misc.cpp resmgr.cpp reshandle.cpp bits.cpp fpset.cpp pose.cpp charcode.cpp dir.cpp)
add_library(spinner STATIC ${CXX_SRC})

install(TARGETS spinner ARCHIVE DESTINATION lib_${TOOL_PREFIX})
file(GLOB HEADERS "./*.hpp")
file(GLOB HEADERS2 "./serialization/*.hpp")
install(FILES ${HEADERS} DESTINATION include/spinner)
install(FILES ${HEADERS2} DESTINATION include/spinner/serialization)

message(STATUS "CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
message(STATUS "CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")
if(NOT DEFINED ARCHITECTURE)
	# テストルーチンについては幾つかの警告を抑制する
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-variable")
	aux_source_directory(tests TEST_SRC)
	add_executable(spn_test main.cpp ${TEST_SRC})
	set(LINK_LIBS spinner boost_regex boost_serialization ${LINK_LIBS} z)
	target_link_libraries(spn_test ${LINK_LIBS})
	install(TARGETS spn_test RUNTIME DESTINATION bin_${TOOL_PREFIX})
endif()
