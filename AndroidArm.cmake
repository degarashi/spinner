# AS_V7AをYESにするとARMv7設定になる

set(ARCHITECTURE armeabi)
set(TOOL_PREFIX arm)
set(ANDROID_PREF "arm-linux-androideabi")
set(ANDROID_TOOLCHAIN "arm-linux-androideabi")
set(ANDROID_ARCH_SHORT "arm")
set(ANDROID_ARCH_LONG "armeabi")
if(AS_V7A)
	set(ANDROID_ARCH_LONG "${ANDROID_ARCH_LONG}-v7a")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv5te")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --sysroot=${ANDROID_NDK_ROOT}/platforms/android-${ANDROID_VER}/arch-arm -fpic -ffunction-sections -funwind-tables -fstack-protector -no-canonical-prefixes")
add_definitions(-D__arm__)
# ARMではExceptionPtrをサポートしてないらしいので無効化するマクロを定義
add_definitions(-DNO_EXCEPTION_PTR)
# ARM or THUMB
if(AS_ARM)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -marm")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fstrict-aliasing -funswitch-loops -finline-limit=300")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthumb")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing -finline-limit=64")
endif()
file(GLOB_RECURSE ANDROID_CMAKE Android.cmake)
include(${ANDROID_CMAKE})

