# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = module
TARGET = pvmp4

LIBDIRS = $(VOB_BASE_DIR)/tools_v2/build/modules/linux_mp4/core/src \
          $(VOB_BASE_DIR)/codecs_v2/omx/omx_m4v/build/make_multithreaded \
          $(VOB_BASE_DIR)/codecs_v2/video/m4v_h263/dec/build/make \
          $(VOB_BASE_DIR)/nodes/pvmp4ffparsernode/build_opencore/make

LIBS = -lpvmp4interface -lomx_m4v_component_lib -lpvmp4decoder -lpvmp4ffparsernode

MODS = -lpvcommon -lpvplayer

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

