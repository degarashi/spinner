set(ARCHITECTURE x86)
set(TOOL_PREFIX x86)
set(ANDROID_PREF "i686-linux-android")
set(ANDROID_TOOLCHAIN "x86")
set(ANDROID_ARCH_SHORT "x86")
set(ANDROID_ARCH_LONG "x86")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --sysroot=${ANDROID_NDK_ROOT}/platforms/android-${ANDROID_VER}/arch-x86 -ffunction-sections -funwind-tables -no-canonical-prefixes -fstack-protector")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fstrict-aliasing -funswitch-loops -finline-limit=300")

include(Android.cmake require)

