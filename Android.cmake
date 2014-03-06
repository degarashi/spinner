set(UNIX 1)
add_definitions(-DUNIX)

set(BOOST_PATH /opt/boost/${ANDROID_ARCH_SHORT})
set(ANDROID_PLATFORM "linux-x86_64")
set(ANDROID_NDK_ROOT "/opt/adt-bundle-${ANDROID_PLATFORM}/ndk")
set(ANDROID_VER "9")
set(CMAKE_TOOLBASE "${ANDROID_NDK_ROOT}/toolchains/${ANDROID_TOOLCHAIN}-4.8/prebuilt/${ANDROID_PLATFORM}/bin/${ANDROID_PREF}-")
set(CMAKE_C_COMPILER "${CMAKE_TOOLBASE}gcc")
set(CMAKE_CXX_COMPILER "${CMAKE_TOOLBASE}g++")
set(CMAKE_LD "${CMAKE_TOOLBASE}ld")
set(CMAKE_AR "${CMAKE_TOOLBASE}ar")
set(CMAKE_RANLIB "${CMAKE_TOOLBASE}ranlib")

# Android用のビルド設定
include_directories(${ANDROID_NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.8/include
					${ANDROID_NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.8/libs/${ANDROID_ARCH_LONG}/include
					${ANDROID_NDK_ROOT}/platforms/android-${ANDROID_VER}/arch-${ANDROID_ARCH_SHORT}/usr/include
					${BOOST_PATH}
					${PROJECT_SOURCE_DIR})
link_directories(${BOOST_PATH}/android_${ARCHITECTURE}/lib
				${ANDROID_NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.8/libs/${ANDROID_ARCH_LONG}
				${ANDROID_NDK_ROOT}/platforms/android-${ANDROID_VER}/arch-${ANDROID_ARCH_SHORT}/usr/lib)
add_definitions(-DANDROID
				-D__ANDROID__
				-DGLIBC
				-D_GLIBCPP_USE_WCHAR_T
				-D_REENTRANT)
# 				-D_LITTLE_ENDIAN
#				-DPAGE_SIZE=2048

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -I${BOOST_PATH}/include")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -ggdb3 -Og -UNDEBUG -fno-omit-frame-pointer -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -O2 -fomit-frame-pointer")

set(LDFLAGS "-no-canonical-prefixes")
set(CMAKE_MODULE_LINKER_FLAGS ${LDFLAGS})
set(CMAKE_SHARED_LINKER_FLAGS ${LDFLAGS})
set(CMAKE_EXE_LINKER_FLAGS ${LDFLAGS})

