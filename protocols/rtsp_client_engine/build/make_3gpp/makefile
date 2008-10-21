# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvrtsp_cli_eng_node_3gpp

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
XINCDIRS += -I../../../../nodes/streaming/config/opencore -I../../../rtsp_parcom/src  -I../../../rtp/src  -I../../../../baselibs/gen_data_structures/src -I ../../../sdp/common/include -I../../../../nodes/streaming/streamingmanager/include -I../../../../baselibs/pvcrypto/src -I../../../rdt_parser/include -I../../../../nodes/streaming/common/include -I../../../../engines/player/src

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
INCSRCDIR = ../../inc

# compose final src list for actual build
SRCS = pvrtsp_client_engine_node.cpp\
	pvrtsp_client_engine_utils.cpp\
	pvrtsp_client_engine_factory.cpp\
	pvrtsp_client_engine_port.cpp

HDRS = pvrtsp_client_engine_node.h\
	pvrtspenginenodeextensioninterface.h\
	pvrtsp_client_engine_utils.h\
	pvrtsp_client_engine_factory.h\
	pvrtsp_client_engine_error_code.h


DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template
