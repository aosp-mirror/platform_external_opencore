# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvm4vencoder

# XINCDIRS += -I ../../common/include

XCXXFLAGS += $(FLAG_COMPILE_WARNINGS_AS_ERRORS)

XCPPFLAGS += -DBX_RC -D__arm__ -DYUV_INPUT -DRGB12_INPUT -DRGB24_INPUT 

ifeq ($(ARCHITECTURE),linux_arm)
XCPPFLAGS += -DPV_ARM_GCC 
endif

SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS =  bitstream_io.cpp \
        combined_encode.cpp \
	datapart_encode.cpp \
	dct.cpp \
        findhalfpel.cpp \
	fastcodemb.cpp \
	fastidct.cpp \
	fastquant.cpp \
	pvm4vencoder.cpp \
	me_utils.cpp \
	mp4enc_api.cpp \
	rate_control.cpp \
	motion_est.cpp \
	motion_comp.cpp \
	sad.cpp \
	sad_halfpel.cpp \
	vlc_encode.cpp \
	vop.cpp 

HDRS =  cvei.h \
	pvm4vencoder.h \
	mp4enc_api.h


DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template
 
