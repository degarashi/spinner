cmake_minimum_required(VERSION 2.6)
project(dgmath)
set(CMAKE_CXX_COMPILER "clang++")
include_directories(${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS_DEBUG "-std=c++11 -g -O0 -stdlib=libc++ -I/usr/local/include/c++/v1")
add_executable(dgmath main.cpp)

install(TARGETS dgmath RUNTIME DESTINATION bin)
