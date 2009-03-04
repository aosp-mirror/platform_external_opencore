# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvstreamingmanagernode

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
XINCDIRS += -I../../../common/include -I../../../jitterbuffernode/include -I../../../jitterbuffernode/include -I../../../medialayernode/include -I../../../../../protocols/rtp/src -I../../../../common/include -I../../../../../protocols/sdp/common/include -I../../../../../fileformats/asf/parser/include -I../../../../../fileformats/rmff_type_specific_parser/src -I../../../../../protocols/rdt_parser/realchallenge/include -I../../../../../pvmi/content_policy_manager/include -I../../../../../pvmi/content_policy_manager/plugins/common/include -I../../../../../nodes/pvrtppacketsourcenode/include -I../../../../../pvmi/pvmf/include -I../../../../../protocols/rtp_payload_parser/rfc_3640/include



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
SRCS = pvmf_sm_node_factory.cpp \
	pvmf_streaming_manager_extension_interface.cpp \
	pvmf_streaming_manager_node.cpp \
	pvmf_streaming_manager_asf_support.cpp \
	pvmf_streaming_manager_real_support.cpp \
	pvmf_streaming_manager_cpm_support.cpp \
	../config/default/streamingmanager_payloadparser_registry.cpp      

HDRS =  pvmf_sm_node_events.h \
	pvmf_sm_node_factory.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

