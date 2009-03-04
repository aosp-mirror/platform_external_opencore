# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = rtppayloadparser_3gpp

# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = " 
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS += 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.
# See notes above regarding overriding project defaults.
#
#XCXXFLAGS += -DTRUE=true -DFALSE=false

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
XINCDIRS += -I ../../rfc_3016/include -I ../../rfc_3267/include -I ../../rfc_3016/include -I ../../rfc_2429/include -I ../../rfc_3984/include -I ../../rfc_3984/src -I ../../rfc_3640/include 
XINCDIRS += -I../../../../pvmi/pvmf/include -I../../../../protocols/sdp/common/include -I ../../../../baselibs/gen_data_structures/src 

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
SRCS =	../rfc_2429/src/h263_payload_parser.cpp \
	../rfc_2429/src/h263_payload_parser_factory.cpp \
	../rfc_3016/src/m4v_payload_parser.cpp \
	../rfc_3016/src/m4v_payload_parser_factory.cpp \
	../rfc_3016/src/m4v_audio_payload_parser.cpp \
	../rfc_3016/src/m4v_audio_payload_parser_factory.cpp \
	../rfc_3267/src/bit_util.cpp \
	../rfc_3267/src/amr_payload_parser.cpp \
	../rfc_3267/src/amr_payload_parser_factory.cpp \
	../rfc_3984/src/h264_payload_parser.cpp \
	../rfc_3984/src/h264_payload_parser_factory.cpp \
	../rfc_3640/src/rfc3640_payload_parser_factory.cpp \
	../rfc_3640/src/rfc3640_payload_parser.cpp \
	 sequence_gen.cpp

HDRS =  bit_util.h \
	payload_parser.h \
	payload_parser_factory.h \
	payload_parser_registry.h \
	../rfc_2429/include/h263_payload_parser.h \
	../rfc_2429/include/h263_payload_parser_factory.h \
	../rfc_3016/include/m4v_payload_parser.h \
	../rfc_3016/include/m4v_payload_parser_factory.h \
	../rfc_3016/include/m4v_audio_payload_parser.h \
	../rfc_3016/include/m4v_audio_payload_parser_factory.h \
	../rfc_3267/include/amr_payload_parser.h \
	../rfc_3267/include/amr_payload_parser_factory.h \
	../rfc_3984/include/h264_payload_parser.h \
	../rfc_3984/include/h264_payload_parser_factory.h \
	../rfc_3640/include/rfc3640_payload_parser.h \
	../rfc_3640/include/rfc3640_payload_parser_factory.h \
	sequence_gen.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

