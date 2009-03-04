include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = omx_queue_lib

XINCDIRS += \
	-I ../../../../../extern_libs_v2/khronos/openmax/include 

SRCDIR = ../../src
INCSRCDIR = ../../src

SRCS = 	pv_omx_queue.cpp 


HDRS =	pv_omx_queue.h 

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

