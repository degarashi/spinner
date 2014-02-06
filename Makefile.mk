# 別途定義しておく変数:
# BUILD_PATH = ビルドパス
# CCPATH = コンパイラが置いてあるパス
PWD					= $(shell pwd)
BUILD_PATH			= /var/tmp/my_build
CCPATH				= ${HOME}/local/bin
INSTALL_PATH		= ${HOME}/baselibs
MY_PATH				= $(CCPATH):${PATH}
LINUX_NJOB			= 5
ANDROID_NJOB		= 2
MINGW_NJOB			= 2
SSE_LEVEL			= 0
LINUX_FLAG			= -DUNIX=TRUE -UWIN32 -DSOUND_API=openal
WIN_FLAG			= -UUNIX -DWIN32=TRUE -DSOUND_API=openal
ANDROID_X86_FLAG	= -DUNIX=TRUE -UWIN32 -DSOUND_API=opensl -DARCHITECTURE=x86
ANDROID_ARM_FLAG	= -DUNIX=TRUE -UWIN32 -DSOUND_API=opensl -DARCHITECTURE=armeabi

CMake = mkdir -p $(1); cd $(1); PATH=$(2); cmake $(PWD) -G 'Unix Makefiles' -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_INSTALL_PREFIX=$(INSTALL_PATH) $(3);
Make = cd $(1); PATH=$(2); make -j$(3);
Install = cd $(1); PATH=$(2); make install;
CMake_Make = $(call CMake,$(1),$(2),$(3)) $(call Make,$(1),$(2),$(4))

LinuxMake = $(call CMake_Make, $(BUILD_PATH)_deb, $(MY_PATH), "$(LINUX_FLAG) -DCMAKE_BUILD_TYPE=$(1)", $(LINUX_NJOB))
LinuxInstall = $(call Install, $(BUILD_PATH)_deb, $(MY_PATH))
MinGWMake = $(call CMake_Make, $(BUILD_PATH)_mingw, $(MY_PATH),  "$(MINGW_FLAG) -DCMAKE_BUILD_TYPE=$(1)", $(MINGW_NJOB))
MinGWInstall = $(call Install, $(BUILD_PATH)_mingw, $(MY_PATH))
AndroidX86Make = $(call CMake_Make, $(BUILD_PATH)_x86, $(MY_PATH), "$(ANDROID_X86_FLAG) -DCMAKE_BUILD_TYPE=$(1)", $(ANDROID_NJOB))
AndroidX86Install = $(call Install, $(BUILD_PATH)_x86, $(MY_PATH))
AndroidArmMake = $(call CMake_Make, $(BUILD_PATH)_arm, $(MY_PATH), "$(ANDROID_ARM_FLAG) -DCMAKE_BUILD_TYPE=$(1)", $(ANDROID_NJOB))
AndroidArmInstall = $(call Install, $(BUILD_PATH)_arm, $(MY_PATH))

linux-d:
	$(call LinuxMake, Debug)
linux:
	$(call LinuxMake, Release)
linux-clean:
	rm -rf $(BUILD_PATH)_deb
linux-d-install: linux-d
	$(call LinuxInstall)
linux-install: linux
	$(call LinuxInstall)

mingw-d:
	$(call MinGWMake, Debug)
mingw:
	$(call MinGWMake, Release)
mingw-clean:
	rm -rf $(BUILD_PATH)_mingw
mingw-d-install: mingw-d
	$(call MinGWInstall)
mingw-install: mingw
	$(call MinGWInstall)

x86-d:
	$(call AndroidX86Make, Debug)
x86:
	$(call AndroidX86Make, Release)
x86-clean:
	rm -rf $(BUILD_PATH)_x86
x86-d-install: x86-d
	$(call AndroidX86Install)
x86-install: x86
	$(call AndroidX86Install)

arm-d:
	$(call AndroidArmMake, Debug)
arm:
	$(call AndroidArmMake, Release)
arm-clean:
	rm -rf $(BUILD_PATH)_arm
arm-d-install: arm-d
	$(call AndroidArmInstall)
arm-install: arm
	$(call AndroidArmInstall)

