set(UNIX 1)
add_definitions(-DUNIX)

set(CMAKE_TOOLBASE "${ANDROID_NDK_ROOT}/toolchains/${ANDROID_TOOLCHAIN}-4.8/prebuilt/${ANDROID_PLATFORM}/bin/${ANDROID_PREF}-")
set(CMAKE_C_COMPILER "${CMAKE_TOOLBASE}gcc")
set(CMAKE_CXX_COMPILER "${CMAKE_TOOLBASE}g++")
set(CMAKE_LD "${CMAKE_TOOLBASE}ld")
set(CMAKE_AR "${CMAKE_TOOLBASE}ar")
set(CMAKE_RANLIB "${CMAKE_TOOLBASE}ranlib")
