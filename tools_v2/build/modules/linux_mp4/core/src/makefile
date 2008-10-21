# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvmp4interface

XINCDIRS += \
	-I $(VOB_BASE_DIR)/codecs_v2/video/m4v_h263/dec/src \
	-I $(VOB_BASE_DIR)/codecs_v2/video/m4v_h263/dec/include \
	-I $(VOB_BASE_DIR)/extern_libs_v2/khronos/openmax/include

SRCDIR = .

# compose final src list for actual build
SRCS =	pvmfmp4nodes.cpp

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

