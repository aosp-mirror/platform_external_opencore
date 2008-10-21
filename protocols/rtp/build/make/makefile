# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = rtprtcp

# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = " 
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS += 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.
# See notes above regarding overriding project defaults.
#
# XCXXFLAGS += 
XCXXFLAGS += $(FLAG_COMPILE_WARNINGS_AS_ERRORS)

# Use XCPPFLAGS to add extra preprocessor directives. Use XINCDIRS
# for other include directories, but please only do this if absolutely
# necessary -- let the makefile system find the include directories if 
# possible.
# See notes above regarding overriding project defaults.
#
# XCPPFLAGS +=  

# Use XINCDIRS for extra include directories. These should be proceeded 
# with "-I" just as they would be when passing to the compiler.
#
# XINCDIRS += 
XINCDIRS += -I../../include

# Use XLIBDIRS for extra library directories. These should be proceeded 
# with "-L" just as they would be when passing to the linker. 
# Used for building executables.
#
# XLIBDIRS += 

# Use XLDFLAGS for extra linker directives.
# Used for building executables.
#
# XLDFLAGS += 

SRCDIR = ../../src
INCSRCDIR = ../../src

SRCS =  rtcp.cpp \
	rtcp_decoder.cpp \
	rtcp_encoder.cpp \
	rtp_decode.cpp \
	rtp_encode.cpp \
	rtp_packet.cpp \
	rtp_packet_impl.cpp \
	rtp_payload.cpp

#INCSRCDIR=../include
HDRS =  rtprtcp.h \
	rtcp.h \
	rtcp_decoder.h \
	rtcp_encoder.h \
	rtcp_constants.h \
	rtp_decode.h \
	rtp_encode.h \
	rtp.h \
	rtp_packet.h \
	rtp_packet_impl.h \
	rtp_payload.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

