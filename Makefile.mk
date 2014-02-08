# 別途定義しておく変数:
# BUILD_PATH = ビルドパス
# CCPATH = コンパイラが置いてあるパス
PWD					:= $(shell pwd)
BUILD_PATH			:= /var/tmp/my_build
INSTALL_PATH		:= ${HOME}/baselibs
LINUX_NJOB			:= 5
ANDROID_NJOB		:= 2
MINGW_NJOB			:= 2
SSE_LEVEL			:= 0
LINUX_FLAG			:= -DSOUND_API=openal -DBUILD_TYPE=Linux
MINGW_FLAG			:= -DSOUND_API=openal -DBUILD_TYPE=MinGW
ANDROID_X86_FLAG	:= -DSOUND_API=opensl -DBUILD_TYPE=AndroidX86
ANDROID_ARM_FLAG	:= -DSOUND_API=opensl -DBUILD_TYPE=AndroidArm

CMake = mkdir -p $(1); cd $(1); cmake $(PWD) -G 'Unix Makefiles' -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_BUILD_TYPE=$(3) -DCMAKE_INSTALL_PREFIX=$(INSTALL_PATH) $(2) ;
Make = cd $(1); make -j$(2);
Install = cd $(1); make install;
Clean = cd $(1); make clean;
CMake_Make = $(call CMake,$(1),$(2),$(3)) $(call Make,$(1),$(4))

LinuxMake = $(call CMake_Make,$(BUILD_PATH)_deb,$(LINUX_FLAG),$(1),$(LINUX_NJOB))
LinuxInstall = $(call Install,$(BUILD_PATH)_deb)
LinuxClean = $(call Clean,$(BUILD_PATH)_deb)
MinGWMake = $(call CMake_Make,$(BUILD_PATH)_mingw,$(MINGW_FLAG),$(1),$(MINGW_NJOB))
MinGWInstall = $(call Install,$(BUILD_PATH)_mingw)
MinGWClean = $(call Clean,$(BUILD_PATH)_mingw)
AndroidX86Make = $(call CMake_Make,$(BUILD_PATH)_x86,$(ANDROID_X86_FLAG),$(1),$(ANDROID_NJOB))
AndroidX86Install = $(call Install,$(BUILD_PATH)_x86)
AndroidX86Clean = $(call Clean,$(BUILD_PATH)_x86)
AndroidArmMake = $(call CMake_Make,$(BUILD_PATH)_arm,$(ANDROID_ARM_FLAG),$(1),$(ANDROID_NJOB))
AndroidArmInstall = $(call Install,$(BUILD_PATH)_arm)
AndroidArmClean = $(call Clean,$(BUILD_PATH)_arm)

linux-d:
	$(call LinuxMake,Debug)
linux:
	$(call LinuxMake,Release)
linux-clean:
	$(call LinuxClean)
linux-d-install: linux-d
	$(call LinuxInstall)
linux-install: linux
	$(call LinuxInstall)

mingw-d:
	$(call MinGWMake,Debug)
mingw:
	$(call MinGWMake,Release)
mingw-clean:
	$(call MinGWClean)
mingw-d-install: mingw-d
	$(call MinGWInstall)
mingw-install: mingw
	$(call MinGWInstall)

x86-d:
	$(call AndroidX86Make,Debug)
x86:
	$(call AndroidX86Make,Release)
x86-clean:
	$(call AndroidX86Clean)
x86-d-install: x86-d
	$(call AndroidX86Install)
x86-install: x86
	$(call AndroidX86Install)

arm-d:
	$(call AndroidArmMake,Debug)
arm:
	$(call AndroidArmMake,Release)
arm-clean:
	$(call AndroidArmClean)
arm-d-install: arm-d
	$(call AndroidArmInstall)
arm-install: arm
	$(call AndroidArmInstall)

