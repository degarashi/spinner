cmake_minimum_required(VERSION 2.8)
project(spinner)

set(SSE_LEVEL 2)
set(CMAKE_BUILD_TYPE "Debug")
include(CMake0.txt require)

# OS依存ファイルの設定
if(${WIN32})
	set(CXX_SRC ${CXX_SRC} dir_depWin.cpp watch_depWin.cpp)
elseif(${UNIX})
	set(CXX_SRC ${CXX_SRC} dir_depLinux.cpp watch_depLinux.cpp)
else()
	message(FATAL_ERROR "couldn't detect OS type")
endif()

set(CXX_SRC ${CXX_SRC} zip.cpp spn_math.cpp spn_math2.cpp spn_math3.cpp spn_math4.cpp pathblock.cpp watch.cpp misc.cpp resmgr.cpp reshandle.cpp bits.cpp fpset.cpp pose.cpp charcode.cpp)
add_library(spinner STATIC ${CXX_SRC})

add_executable(spn_test unittest.cpp main.cpp)
target_link_libraries(spn_test spinner boost_regex)
