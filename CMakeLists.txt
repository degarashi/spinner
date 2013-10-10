cmake_minimum_required(VERSION 2.8)
project(spinner)
set(CMAKE_CXX_COMPILER "clang++")

# Use SSE instructions
set(SSE_LEVEL 2)
# Use NEON instructions
# set(NEON_LEVEL 1)

if(${SSE_LEVEL} GREATER 0)
	add_definitions(-DSSE_LEVEL=${SSE_LEVEL})
	message("configured as to using SSE-" ${SSE_LEVEL} " instructions...")
elseif(${NEON_LEVEL} GREATER 0)
	add_definitions(-DNEON_LEVEL=${NEON_LEVEL})
	message("configured as to using NEON-" ${NEON_LEVEL} " instructions...")
endif()

include_directories(${PROJECT_SOURCE_DIR})
include_directories("/usr/local/include/c++/4.8.1")
include_directories("/usr/local/include/c++/4.8.1/i686-pc-linux-gnu")
link_directories("/usr/local/lib/gcc/i686-pc-linux-gnu/4.8.1")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -std=c++11 -ggdb3 -O0 -msse2")
set(CMAKE_CXX_FLAGS_RELEASE "-std=c++11 -O2 -msse2")
set(CMAKE_BUILD_TYPE "Debug")
add_library(spinner STATIC spn_math4.cpp spn_math3.cpp misc.cpp resmgr.cpp reshandle.cpp bits.cpp spn_math.cpp spn_math2.cpp fpset.cpp pose.cpp charcode.cpp)

add_executable(spn_test unittest.cpp main.cpp)
target_link_libraries(spn_test spinner boost_system)
