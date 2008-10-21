#
# This file contains the architecture specific 
# definitions for linux_arm platform.
#

AR_ARGS = rvl
AR = arm-eabi-ar

CC = arm-eabi-gcc
CXX = arm-eabi-g++
CO = -c -o # make sure to leave a space at the end

export RANLIB = arm-eabi-ranlib

XCPPFLAGS += -fomit-frame-pointer -fno-strict-aliasing -finline-limit=150 -msoft-float -O2 -march=armv5te -W -fno-rtti -fno-exceptions -mtune=xscale -fpic -mthumb-interwork -funwind-tables -finline-functions -fno-inline-functions-called-once -funswitch-loops -fgcse-after-reload -frerun-cse-after-loop -frename-registers -Wstrict-aliasing=2

# The following flag is needed for all vobs except codecs_v2.
# Let's use this by default for now.
XCPPFLAGS += -mthumb

# The following flag has been used for Android to avoid linkage errors
XCPPFLAGS += -mlong-calls 

COMPFLAGS = -Wall -Wno-non-virtual-dtor -Wno-multichar

# include the common engine header files in the search path
XINCDIRS += \
	-I $(ANDROID_BASE)/device/include \
	-I $(ANDROID_BASE)/device/system/libm/include \
	-I $(ANDROID_BASE)/device/system/bionic/include \
	-I $(ANDROID_BASE)/device/system/bionic/arch-arm/include \
	-I $(ANDROID_BASE)/device/system/bionic/arch-arm/include/machine \
	-I $(ANDROID_BASE)/device/system/libstdc++/include \
	-I $(KERNEL_HEADERS) \
	-I $(ANDROID_BASE)/device/system/libthread_db/include \
	-I $(ANDROID_BASE)/device/system/libstdc++ \

# Use XLIBDIRS for extra library directories. These should be proceeded 
# with "-L" just as they would be when passing to the linker. 
# Used for building executables.
XLIBDIRS += \
	-L $(ANDROID_BASE)/../ \
	-L $(ANDROID_BASE)/../toolchain-eabi-4.2.1/lib \
	-L $(ANDROID_BASE)/../toolchain-eabi-4.2.1/arm-eabi/lib \
	-lc

