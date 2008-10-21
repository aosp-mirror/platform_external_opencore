include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = omx_amr_component_lib



XINCDIRS += \
    -I ../../../../../extern_libs_v2/khronos/openmax/include \
    -I ../../../../audio/gsm_amr/amr_nb/dec/src \
    -I ../../../../audio/gsm_amr/amr_nb/dec/include \
    -I ../../../../audio/gsm_amr/amr_nb/common/include \
    -I ../../../../audio/gsm_amr/common/dec/include \
    -I ../../../../audio/gsm_amr/amr_wb/common/include \
    -I ../../../../audio/gsm_amr/amr_wb/dec/include \
    -I ../../../../audio/gsm_amr/amr_wb/dec/src

SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS =  amr_decode_frame.cpp \
    omx_amr_component.cpp \
    d_homing.cpp


HDRS =  amr_dec.h \
    omx_amr_component.h \
    d_homing.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

