# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = protocolenginenode

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
XCPPFLAGS += -DPV_PROTOCOL_ENGINE_NODE_PROGRESSIVE_DOWNLOAD_ENABLED -DPV_PROTOCOL_ENGINE_NODE_PROGRESSIVE_STREAMING_ENABLED 

# Use XINCDIRS for extra include directories. These should be proceeded 
# with "-I" just as they would be when passing to the compiler.
#
# Added this to for osclconfig.h
XINCDIRS += -I $(VOB_BASE_DIR)/protocols/http_paracom/include -I $(VOB_BASE_DIR)/nodes/common/include -I $(VOB_BASE_DIR)/baselibs/pv_mime_utils/src -I $(VOB_BASE_DIR)/protocols/sdp/common/include -I $(VOB_BASE_DIR)/protocols/sdp/parser/include -I $(VOB_BASE_DIR)/baselibs/gen_data_structures/src -I $(VOB_BASE_DIR)/baselibs/media_data_structures/src -I $(VOB_BASE_DIR)/fileformats/pvx/parser/include
XINCDIRS += -I ../../config/$(BUILD_ARCH) -I ../../config/linux


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
SRCS = 	pvdl_config_file.cpp \
	pvmf_protocol_engine_common.cpp \
	pvmf_protocol_engine_download_common.cpp \
	pvmf_protocol_engine_progressive_download.cpp \
	pvmf_protocol_engine_node_common.cpp \
	pvmf_protocol_engine_node_download_common.cpp \
	pvmf_protocol_engine_node_progressive_download.cpp \
	pvmf_protocol_engine_node_progressive_streaming.cpp \
	pvmf_protocol_engine_factory.cpp \
	pvmf_protocol_engine_port.cpp
	

HDRS =  pvmf_protocol_engine_factory.h \
	pvmf_protocol_engine_defs.h \
	pvmf_protocol_engine_node_extension.h \
	pvmf_protocol_engine_command_format_ids.h \
	pvmf_protocol_engine_node_events.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template
