cmake_minimum_required(VERSION 2.8)
project(spinner)
set(CMAKE_CXX_COMPILER "clang++")
include_directories(${PROJECT_SOURCE_DIR})
include_directories("/usr/local/include/c++/v1/"
					"/usr/local/include/c++/v1/i686-pc-linux-gnu/")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -std=c++11 -stdlib=libc++ -ggdb3 -O0 -msse2")
set(CMAKE_CXX_FLAGS_RELEASE "-std=c++11 -stdlib=libc++ -O2 -msse2")
set(CMAKE_BUILD_TYPE "Debug")
add_library(spinner STATIC charcode.cpp misc.cpp resmgr.cpp reshandle.cpp bits.cpp spn_math.cpp spn_math2.cpp fpset.cpp pose.cpp)

add_executable(spn_test main.cpp)
target_link_libraries(spn_test spinner boost_system supc++)
