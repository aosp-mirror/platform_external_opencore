include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = omx_mp3_component_lib



XINCDIRS += \
    -I ../../../../../extern_libs_v2/khronos/openmax/include \
    -I ../../../../audio/mp3/pv_dec/src \
    -I ../../../../audio/mp3/pv_dec/include


SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS =  mp3_dec.cpp \
    omx_mp3_component.cpp \
    mp3_timestamp.cpp  


HDRS =  mp3_dec.h \
    omx_mp3_component.h \
    mp3_timestamp.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

