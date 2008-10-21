# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = module
TARGET = pvdownloadreg

LIBDIRS = $(VOB_BASE_DIR)/tools_v2/build/modules/linux_download/node_registry/src

LIBS = -lpvdownloadreginterface

MODS = -lpvcommon -lpvplayer

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

