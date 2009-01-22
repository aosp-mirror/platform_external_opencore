
CXX := arm-eabi-g++
CC := arm-eabi-gcc
LINK := $(CXX)
SHARED_LINK := $(CXX)
SHARED_PRELINK := $(ANDROID_BASE)/tools/apriori 
SHARED_PRELINK_FLAGS := --quiet --prelinkmap $(CFG_DIR)/prelink-linux-arm.map --locals-only
STRIP := $(ANDROID_BASE)/tools/soslim
STRIP_FLAGS := --strip --shady --quiet
AR := arm-eabi-ar

# overwrite RANLIB set in linux.mk
export RANLIB := arm-eabi-ranlib

# This toolchain supports linux-arm v5
DEFAULT_CPP_ASM_FLAGS := -DPV_ARM_GCC_V5
PROCESSOR := arm

# include the common engine header files in the search path
INCDIRS += \
    -I $(ANDROID_BASE)/headers/include \
    -I $(ANDROID_BASE)/headers/system/bionic/arch-arm/include \
    -I $(ANDROID_BASE)/headers/system/bionic/include \
    -I $(ANDROID_BASE)/headers/system/libstdc++/include \
    -I $(ANDROID_BASE)/headers/system/bionic/kernel/common \
    -I $(ANDROID_BASE)/headers/system/bionic/kernel/arch-arm \
    -I $(ANDROID_BASE)/headers/system/libm/include \

# Compiler options
CPPFLAGS += -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 -msoft-float -O2 -march=armv5te -fno-rtti -fno-exceptions -mtune=xscale -fpic -mthumb-interwork -ffunction-sections -funwind-tables -finline-functions -fno-inline-functions-called-once -fgcse-after-reload -frerun-cse-after-loop -frename-registers -Wstrict-aliasing=2 -fstack-protector -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__ -MD -include $(ANDROID_BASE)/headers/include/arch/linux-arm/AndroidConfig.h -DANDROID -fmessage-length=0 -DSK_RELEASE -DNDEBUG -DUDEBUG -g -fvisibility-inlines-hidden -Os -DENABLE_MEMORY_PLAYBACK -fno-rtti

# Idea is to build only libraries in arm mode and the rest in thumb mode
OPTIMIZE_FOR_SIZE := -mthumb

CXXFLAGS = -W -Wall -Wno-unused -Wno-non-virtual-dtor -Wno-multichar

override SYSLIBS = -lc -lm -ldl -lstdc++

SONAME_ARG := -Wl,-T,$(ANDROID_BASE)/config/armelf.xsc -Wl,--gc-sections -L$(ANDROID_BASE)/prebuilt/obj/lib -Wl,--whole-archive -Wl,-h,
SHARED_PRE_LDFLAGS := -nostdlib -Wl,-shared,-Bsymbolic 
SHARED_POST_LDFLAGS := -Wl,--no-whole-archive $(SYSLIBS) -Wl,--no-undefined $(ANDROID_BASE)/toolchain-eabi-4.2.1/lib/gcc/arm-eabi/4.2.1/interwork/libgcc.a

# Compiler specific libraries locations
LIB_DIRS = \
    -L$(ANDROID_BASE)/prebuilt/obj/lib \
    -L$(ANDROID_BASE)/toolchain-eabi-4.2.1/lib \
    -L$(ANDROID_BASE)/toolchain-eabi-4.2.1/arm-eabi/lib


PRE_LDFLAGS += \
   $(ANDROID_BASE)/toolchain-eabi-4.2.1/lib/gcc/arm-eabi/4.2.1/crtbegin_dynamic.o

POST_LDFLAGS += \
   -Wl,--no-undefined \
   $(ANDROID_BASE)/toolchain-eabi-4.2.1/lib/gcc/arm-eabi/4.2.1/interwork/libgcc.a \
   $(ANDROID_BASE)/prebuilt/obj/lib/crtend_android.o

export BINDING = -nostdlib -Bdynamic -Wl,-T,$(ANDROID_BASE)/config/armelf.x \
        -Wl,-dynamic-linker,/system/bin/linker -Wl,--gc-sections -Wl,-z,nocopyreloc \
        -Wl,-rpath-link=$(ANDROID_BASE)/prebuilt/obj/lib


ifneq ($(strip $(LOCAL_BYPASS_PRELINKING)),true)
define prelink_and_strip
  $(quiet) $(SHARED_PRELINK) $(SHARED_PRELINK_FLAGS) $1
  $(quiet) $(STRIP) $(STRIP_FLAGS) $1
endef
else
define prelink_and_strip
endef
endif

define generate_shared_lib
  $(quiet) $(SHARED_LINK) $(SHARED_PRE_LDFLAGS) $(SONAME_ARG)$(notdir $1) -o $1 $2 $(SHARED_POST_LDFLAGS)
  $(call prelink_and_strip,$1)
endef


define generate_prog
  $(quiet) $(LINK) $(BINDING) $(OUTLINKFLAG)$1 $(filter $(LIBCOMPFLAG)%,$($2_LDFLAGS)) \
    $(PRE_LDFLAGS) $(XOBJECTS) $($2_compiled_objs)  $(filter %.a,$(filter-out $(LIBCOMPFLAG)%,$($2_LDFLAGS))) \
    $(POST_LDFLAGS) $(patsubst lib%.so,-l%,$(notdir $(filter-out %.a,$(filter-out $(LIBCOMPFLAG)%,$($2_LDFLAGS))))) $(SYSLIBS)
  $(quiet) $(STRIP) $(STRIP_FLAGS) $1
endef

## END FILE 

