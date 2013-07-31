cmake_minimum_required(VERSION 2.8)
project(spinner)
set(CMAKE_CXX_COMPILER "i386-mingw32-g++")
set(CMAKE_AR "i386-mingw32-ar")
set(CMAKE_RANLIB "i386-mingw32-ranlib")
include_directories("/home/slice/local/i386-mingw32/include/c++/4.8.1/"
					"/home/slice/local/i386-mingw32/include/"
					"/home/slice/local/include/"
					${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -std=c++11 -ggdb3 -O0 -msse2")
set(CMAKE_CXX_FLAGS_RELEASE "-std=c++11 -O3 -msse2")
set(CMAKE_BUILD_TYPE "Release")
add_library(spinner STATIC misc.cpp resmgr.cpp reshandle.cpp bits.cpp spn_math.cpp spn_math2.cpp fpset.cpp pose.cpp)
