set(UNIX 1)
add_definitions(-DUNIX)
set(TOOL_PREFIX deb)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEPEND_CFLAG}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb3")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -O0 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(LINK_LIBS boost_thread boost_system pthread)

