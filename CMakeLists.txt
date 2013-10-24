cmake_minimum_required(VERSION 2.8)
project(spinner)

set(SSE_LEVEL 2)
set(CMAKE_BUILD_TYPE "Debug")
include(CMake0.txt require)

add_library(spinner STATIC spn_math4.cpp spn_math3.cpp misc.cpp resmgr.cpp reshandle.cpp bits.cpp spn_math.cpp spn_math2.cpp fpset.cpp pose.cpp charcode.cpp)

add_executable(spn_test unittest.cpp main.cpp)
target_link_libraries(spn_test spinner boost_system)
