cmake_minimum_required(VERSION 2.8)
project(spinner)
# KDevelopの変数設定が機能していないのでターゲットによってコメントアウトを切り替える
set(SSE_LEVEL 2)
# set(ARCHITECTURE x86)
include(CMakeCommon.txt require)
include(CMakeCompiler.txt require)

# OS依存ファイルの設定
if(${WIN32})
	set(CXX_SRC ${CXX_SRC} dir_depWin.cpp watch_depWin.cpp)
elseif(${UNIX})
	set(CXX_SRC ${CXX_SRC} dir_depLinux.cpp watch_depLinux.cpp)
else()
	message(FATAL_ERROR "couldn't detect OS type")
endif()

set(CXX_SRC ${CXX_SRC} error.cpp adaptstd.cpp ziptree.cpp zip.cpp spn_math.cpp spn_math2.cpp spn_math3.cpp spn_math4.cpp pathblock.cpp watch.cpp misc.cpp resmgr.cpp reshandle.cpp bits.cpp fpset.cpp pose.cpp charcode.cpp)
add_library(spinner STATIC ${CXX_SRC})

install(TARGETS spinner ARCHIVE DESTINATION lib_${TOOL_PREFIX})
file(GLOB HEADERS "./*.hpp")
install(FILES ${HEADERS} DESTINATION include/spinner)

if(NOT DEFINED ${ARCHITECTURE})
	add_executable(spn_test main.cpp unittest.cpp)
	target_link_libraries(spn_test spinner boost_regex boost_serialization)
	install(TARGETS spn_test RUNTIME DESTINATION bin_${TOOL_PREFIX})
endif()
