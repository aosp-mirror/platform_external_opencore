# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvsdpparser

# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = " 
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS += 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.
# See notes above regarding overriding project defaults.
#
# XCXXFLAGS += -DUSE_UNICODE  -D_POSIX_PTHREAD_SEMANTICS   #-D_POSIX_SOURCE

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
# Added this to for osclconfig.h
#XINCDIRS += -I../../../config/$(BUILD_ARCH)  -I../../../config/shared
XINCDIRS += -I../../../common/include -I ../../../../../baselibs/gen_data_structures/src

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
INCSRCDIR = ../../include

# compose final src list for actual build
SRCS = aacmediainfoparser.cpp\
	amrmediainfoparser.cpp\
	sdp_mediaparser_factory.cpp\
	sdpparser.cpp\
	basemediainfoparser.cpp\
	sdp_parsing_utils.cpp\
	evrcmediainfoparser.cpp\
	sessioninfoparser.cpp\
	h263mediainfoparser.cpp\
	rfc3640mediainfoparser.cpp\
	h264mediainfoparser.cpp\
	still_imagemediainfoparser.cpp\
	m4vmediainfoparser.cpp\
	pcmamediainfoparser.cpp \
	pcmumediainfoparser.cpp
	

HDRS = aac_media_info_parser.h\
	sdp_mediaparser_factory.h\
	amr_media_info_parser.h\
	sdp_mediaparser_registry.h\
	asf_media_info_parser.h\
	sdp_parser.h\
	base_media_info_parser.h\
	sdp_parsing_utils.h\
	evrc_media_info_parser.h\
	session_info_parser.h\
	h263_media_info_parser.h\
	rfc3640_media_info_parser.h\
	h264_media_info_parser.h\
	still_image_media_info_parser.h\
	m4v_media_info_parser.h
	
DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template
