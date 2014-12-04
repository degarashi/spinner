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

set(CXX_SRC ${CXX_SRC} color.cpp random.cpp filetree.cpp error.cpp adaptstd.cpp ziptree.cpp zip.cpp spn_math.cpp spn_math2.cpp spn_math3.cpp spn_math4.cpp pathblock.cpp watch.cpp misc.cpp resmgr.cpp reshandle.cpp bits.cpp fpset.cpp pose.cpp charcode.cpp dir.cpp)
add_library(spinner STATIC ${CXX_SRC})

install(TARGETS spinner ARCHIVE DESTINATION lib_${TOOL_PREFIX})
file(GLOB_RECURSE HEADERS "*.hpp")

# 除外するサブディレクトリを設定
set(EXCLUDE_DIR "tests")
# ヘッダのサブディレクトリを集計
foreach(HEADER IN LISTS HEADERS)
	string(REGEX MATCH ".*/spinner/([^/]+)/.*" DIRNAME ${HEADER})
	if(DIRNAME)
		set(DIRNAME ${CMAKE_MATCH_1})
		set(EXCLUDE_FLAG FALSE)
		foreach(E_DIR IN LISTS EXCLUDE_DIR)
			string(REGEX MATCH "${E_DIR}" E_NAME ${DIRNAME})
			if(E_NAME)
				set(EXCLUDE_FLAG TRUE)
			endif()
		endforeach()
		if(NOT EXCLUDE_FLAG)
			list(APPEND HEADERS_DIR ${DIRNAME})
		endif()
	endif()
endforeach()
list(REMOVE_DUPLICATES HEADERS_DIR)
# ルートディレクトリ内のヘッダを出力
# サブディレクトリ内のヘッダを出力
foreach(DIRNAME IN LISTS HEADERS_DIR)
	unset(HEADER_LIST)
	foreach(HEADER IN LISTS HEADERS)
		string(REGEX MATCH ".*/spinner/(${DIRNAME})/.*" DUMMY ${HEADER})
		if(CMAKE_MATCH_1)
			list(APPEND HEADER_LIST ${HEADER})
		else()
			list(APPEND ROOTHEADER_LIST ${HEADER})
		endif()
	endforeach()
	install(FILES ${HEADER_LIST} DESTINATION include/spinner/${DIRNAME})
endforeach()
install(FILES ${ROOTHEADER_LIST} DESTINATION include/spinner)

message(STATUS "CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
message(STATUS "CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")
if(NOT DEFINED ARCHITECTURE)
	aux_source_directory(tests TEST_SRC)
	add_executable(spn_test main.cpp ${TEST_SRC})
	set(LINK_LIBS
				spinner
				boost_regex
				boost_serialization
				${LINK_LIBS}
				gtest
				pthread
				z)
	target_link_libraries(spn_test ${LINK_LIBS})
	install(TARGETS spn_test RUNTIME DESTINATION bin_${TOOL_PREFIX})
	add_test(NAME only_test
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMAND spn_test)
	enable_testing()
endif()
