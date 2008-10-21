include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pv_avc_common_lib

XCXXFLAGS += $(FLAG_COMPILE_WARNINGS_AS_ERRORS)

SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS =  deblock.cpp \
	dpb.cpp \
	fmo.cpp \
	mb_access.cpp \
	reflist.cpp

HDRS =  avcapi_common.h avcint_common.h avclib_common.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template
