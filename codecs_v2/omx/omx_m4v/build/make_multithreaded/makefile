include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = omx_m4v_component_lib

XINCDIRS += \
	-I ../../../../../extern_libs_v2/khronos/openmax/include \
	-I ../../../../video/m4v_h263/dec/src \
	-I ../../../../video/m4v_h263/dec/include

SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS = 	m4v_io.cpp \
	omx_mpeg4_component.cpp \
	mpeg4_dec.cpp 


HDRS =	m4v_io.h \
	omx_mpeg4_component.h \
	mpeg4_dec.h \
	mpeg4video_frame.h


DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

