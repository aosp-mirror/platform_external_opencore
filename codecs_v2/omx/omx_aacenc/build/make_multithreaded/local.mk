# Get the current local path as the first operation
LOCAL_PATH := $(call get_makefile_dir)

# Clear out the variables used in the local makefiles
include $(MK)/clear.mk

TARGET := omx_aacenc_component_lib

XCXXFLAGS += $(FLAG_COMPILE_WARNINGS_AS_ERRORS)

OPTIMIZE_FOR_PERFORMANCE_OVER_SIZE := true

XINCDIRS += \
  ../../../../../extern_libs_v2/khronos/openmax/include \
  ../../../../audio/aac/enc/src \
  ../../../../audio/aac/enc/include 


SRCDIR := ../../src
INCSRCDIR := ../../include

SRCS := aac_enc.cpp \
	omx_aacenc_component.cpp


HDRS := aac_enc.h \
	omx_aacenc_component.h
	



include $(MK)/library.mk

