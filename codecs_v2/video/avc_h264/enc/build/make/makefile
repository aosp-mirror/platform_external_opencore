#Include the project-wide makefile to get
# appropriate definitions

include $(PV_TOP)/makefile.pv


# BE CAREFUL NOT TO HAVE ANY TRAILING WHITESPACE FOR THE NEXT 3 LINES BELOW OR IT WILL NOT WORK!
WHAT = library
TARGET = pvavch264enc
# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = "
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS = 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.
# See notes above regarding overriding project defaults.
#
# XCXXFLAGS += -Wall -g -DUSE_UNICODE -D_POSIX_PTHREAD_SEMANTICS

# Use XCPPFLAGS to add extra preprocessor directives. Use XINCDIRS
# for other include directories, but please only do this if absolutely
# necessary -- let the makefile system find the include directories if
# possible.
# See notes above regarding overriding project defaults.
#
#XCPPFLAGS += -DORIGINAL_TYPEDEF_H -DOSCL_BASE_H_INCLUDED -DOSCL_TYPES_H_INCLUDE -DOSCL_ASSERT_H_INCLUDED -DOSCL_MEM_BASIC_FUNCTIONS_H -DOSCL_MEM_IMP_H
XCPPFLAGS += -D__arm__ -DYUV_INPUT -DRGB24_INPUT -DRGB12_INPUT

# Use XINCDIRS for extra include directories. These should be proceeded
# with "-I" just as they would be when passing to the compiler.
#
# XINCDIRS +=
XINCDIRS += -I$(SDK_LOCAL)/installed_include
XINCDIRS += -I../../../common/include

# Use XLIBDIRS for extra library directories. These should be proceeded
# with "-L" just as they would be when passing to the linker.
# Used for building executables.
#
XLIBDIRS += 

# Use XLDFLAGS for extra linker directives.
# Used for building executables.
#
# XLDFLAGS +=

SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS =  avcenc_api.cpp \
	bitstream_io.cpp \
	block.cpp \
	findhalfpel.cpp \
	header.cpp \
	init.cpp \
	intra_est.cpp \
	motion_comp.cpp \
	motion_est.cpp \
	pvavcencoder.cpp \
	pvavcencoder_factory.cpp \
	rate_control.cpp \
	residual.cpp \
	sad.cpp \
	sad_halfpel.cpp \
	slice.cpp \
	vlc_encode.cpp

HDRS = pvavcencoder.h \
       pvavcencoder_factory.h \
       pvavcencoderinterface.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

