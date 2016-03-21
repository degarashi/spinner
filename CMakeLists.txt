cmake_minimum_required(VERSION 2.8)
project(spinner)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED system thread regex serialization)
find_package(GTest REQUIRED)
find_package(ZLIB REQUIRED)
# Linux: BUILD_TYPE = Linux
# MinGW: BUILD_TYPE = MinGW
# Android:	BUILD_TYPE = AndroidX86 or AndroidArm
message(STATUS ${BUILD_TYPE})
include("crosscompile/${BUILD_TYPE}.cmake" require)

# windows依存ファイル
set(DEPENDANT_SRC_WIN dir_depWin.cpp watch_depWin.cpp)
# unix依存ファイル
set(DEPENDANT_SRC_UNIX dir_depLinux.cpp watch_depLinux.cpp)
set(OS_DEPENDANT_SRC ${DEPENDANT_SRC_WIN} ${DEPENDANT_SRC_UNIX})
# 依存ファイル以外のソースファイルを収集
file(GLOB LIBSRC_FILE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
list(REMOVE_ITEM LIBSRC_FILE main.cpp ${OS_DEPENDANT_SRC})
# OS依存ファイルの設定
if(${WIN32})
	set(LIBSRC_FILE ${LIBSRC_FILE} ${DEPENDANT_SRC_WIN})
elseif(${UNIX})
	set(LIBSRC_FILE ${LIBSRC_FILE} ${DEPENDANT_SRC_UNIX})
else()
	message(FATAL_ERROR "couldn't detect OS type")
endif()
file(GLOB_RECURSE HEADERS "*.hpp")

add_library(spinner STATIC ${LIBSRC_FILE})
install(TARGETS spinner ARCHIVE DESTINATION lib_${TOOL_PREFIX})

set(COPYHEADERS_EXCLUDE_DIR "")		# ここに除外したいヘッダディレクトリを指定
set(COPYHEADERS_MY_DIR "spinner")
include(crosscompile/CopyHeaders.cmake require)
include_directories(
	${Boost_INCLUDE_DIRS}
	${GTEST_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
)

message(STATUS "CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
message(STATUS "CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")
if(NOT DEFINED ARCHITECTURE)
	aux_source_directory(tests TEST_SRC)
	add_executable(spn_test main.cpp ${TEST_SRC})
	set(LINK_LIBS
				spinner
				${Boost_LIBRARIES}
				${GTEST_LIBRARIES}
				${CMAKE_THREAD_LIBS_INIT}
				${ZLIB_LIBRARIES})
	target_link_libraries(spn_test ${LINK_LIBS})
	install(TARGETS spn_test RUNTIME DESTINATION bin_${TOOL_PREFIX})
	add_test(NAME only_test
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMAND spn_test)
	enable_testing()
endif()
