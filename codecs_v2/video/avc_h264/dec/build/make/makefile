include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvavcdecoder

XCXXFLAGS += $(FLAG_COMPILE_WARNINGS_AS_ERRORS)

SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS =  avcdec_api.cpp \
	avc_bitstream.cpp \
	header.cpp \
	itrans.cpp \
	pred_inter.cpp \
	pred_intra.cpp \
	pvavcdecoder.cpp \
	pvavcdecoder_factory.cpp \
	residual.cpp \
	slice.cpp \
	vlc.cpp

HDRS =  pvavcdecoder.h \
	pvavcdecoder_factory.h \
	pvavcdecoderinterface.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template
