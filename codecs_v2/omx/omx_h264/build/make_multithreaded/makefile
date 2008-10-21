include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = omx_avc_component_lib


XINCDIRS += \
	-I ../../../../../extern_libs_v2/khronos/openmax/include \
	-I ../../../../video/avc_h264/dec/src \
	-I ../../../../video/avc_h264/dec/include

SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS = 	avc_dec.cpp \
	omx_avc_component.cpp 


HDRS =	avc_dec.h \
	omx_avc_component.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

