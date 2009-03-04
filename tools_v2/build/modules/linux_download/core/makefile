# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = module
TARGET = pvdownload

LIBDIRS = $(VOB_BASE_DIR)/tools_v2/build/modules/linux_download/core/src \
		  $(VOB_BASE_DIR)/nodes/pvdownloadmanagernode/build/make

LIBS = -lpvdownloadinterface -lpvdownloadmanagernode

MODS = -lpvnet_support -lpvcommon

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

