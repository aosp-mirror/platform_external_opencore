#
# This Makefile template should contain the project
# specific information to adapt the templates to 
# other projects.

ifeq ($(strip $(PV_DEFINES)),)
  PV_DEFINES := $(shell $(MK)/../bin/pv_defines $(BUILD_ARCH)) 
endif


# Add pv_defines to cppflags
CPPFLAGS += $(PV_DEFINES)



ifeq ($(INSTALL),)
  INSTALL = install
endif

ifeq ($(SED),)
  SED = sed
endif

ifeq ($(KEEP_OBJ),)
  KEEP_OBJ = 0
endif

export TOP := $(PROJECT)


# Define the FIND_DIR utility used for finding directories in
# the project directory structure
FIND_DIR = $(MK)/../bin/find_dir
FIND_FILE = $(MK)/../bin/find_file
GEN_DEPS = $(MK)/../bin/gen_deps
SHOW_DOC = $(MK)/../bin/show_doc



#
# X11
#  XINCDIRS += -I/usr/X11R6/include -I/usr/local/Motif-2.0/include
#  XLIBDIRS += -L/usr/X11R6/lib -L/usr/local/Motif-2.0/lib
ifeq ($(BUILD_ARCH),linux)
  POST_LIBDIRS += -L/usr/lib -L/usr/local/lib
  ifdef X11
    POST_INCDIRS += -I/usr/X11R6/include 
    POST_LIBDIRS += -L/usr/X11R6/lib
    ifdef XMT
      POST_INCDIRS += -I$(XMT)/include
      POST_LIBDIRS += -L$(XMT)/lib
    endif
  endif
endif

# Set the default search directories for project
ifneq ($(strip $(INCSRCDIR)),)
  INCDIRS += -I$(INCSRCDIR)
endif
INCDIRS += -I$(SRCDIR)
INCDIRS += $(shell $(FIND_DIR) -I include $(TOP))
INCDIRS += $(shell $(FIND_DIR) -I installed_include $(TOP))

LIB_DIRS := $(shell $(FIND_DIR) -L lib/$(BUILD_ARCH) $(TOP))

#
# LINUX-NJ
#
ifeq ($(ARCHITECTURE),linux_nj)
   override SYSLIBS = -lc -lm -ldl -lstdc++
   PRE_LDFLAGS += \
   $(ANDROID_BASE)/../toolchain-eabi-4.2.1/lib/gcc/arm-eabi/4.2.1/crtbegin_static.o
   POST_LDFLAGS += \
   $(ANDROID_BASE)/../toolchain-eabi-4.2.1/lib/gcc/arm-eabi/4.2.1/libgcc.a \
   $(ANDROID_BASE)/../toolchain-eabi-4.2.1/lib/gcc/arm-eabi/4.2.1/crtend.o
endif

# HAS_OSCL_LIB_SUPPORT is required for dynamic loading
# PV_LINUX_BUILD is required to differentiate between the Android build and
# the PV Linux build
ifeq ($(FORMAT),nj)
	CPPFLAGS += -DHAS_OSCL_LIB_SUPPORT -DPV_LINUX_BUILD
endif

#
# CPP - Any one will do
#
CPP=/lib/cpp


#
# Default filetypes for the clean rule.  Just add things like editor backups
# here -- the targets will add whatever's appropriate.  Also add the dependency
# files, too.
#
CLEAN_DEFAULTS=*.BAK *~ *% *.tmp*

