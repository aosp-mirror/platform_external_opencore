include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = omx_m4v_component_interface

XINCDIRS += \
	-I ../../../../../../codecs_v2/omx/omx_m4v/include \
	-I ../../../../../../codecs_v2/omx/omx_m4v/src \
	-I ../../../../../../extern_libs_v2/khronos/openmax/include \
	-I ../../../../../../codecs_v2/video/m4v_h263/dec/src \
	-I ../../../../../../codecs_v2/video/m4v_h263/dec/include

SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS = 	omx_m4v_component_interface.cpp


HDRS = omx_m4v_component_interface.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

