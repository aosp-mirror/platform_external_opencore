# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = module
TARGET = pvrtspreg

LIBDIRS = $(VOB_BASE_DIR)/tools_v2/build/modules/linux_rtsp/node_registry/src

LIBS = -lpvrtspreginterface

MODS = -lpvcommon -lpvplayer

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

