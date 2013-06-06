cmake_minimum_required(VERSION 2.8)
project(dgmath)
set(CMAKE_CXX_COMPILER "clang++")
include_directories(${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS_DEBUG "-std=c++11 -g -O0 -msse2")
add_library(spinner STATIC geom2D.cpp spn_math.cpp fpset.cpp pose.cpp)

add_executable(spn_test main.cpp)
target_link_libraries(spn_test spinner)
