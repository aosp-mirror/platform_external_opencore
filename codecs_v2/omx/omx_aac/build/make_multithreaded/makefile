include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = omx_aac_component_lib

XCPPFLAGS += -DAAC_PLUS -DHQ_SBR -DPARAMETRICSTEREO

XINCDIRS += \
	-I ../../../../../extern_libs_v2/khronos/openmax/include \
	-I ../../../../audio/aac/dec/src \
	-I ../../../../audio/aac/dec/include

SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS =  aac_decode_frame.cpp \
        omx_aac_component.cpp \
        aac_timestamp.cpp  


HDRS =  aac_dec.h \
        omx_aac_component.h \
        aac_timestamp.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

