# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvmf

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
#XINCDIRS += -I../../../config/$(BUILD_ARCH)  -I../../../config/shared


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
SRCS =	pvmf_format_type.cpp \
	pvmf_pool_buffer_allocator.cpp \
	pvmf_sync_util_data_queue.cpp \
	pvmf_media_cmd.cpp \
	pvmf_media_data.cpp \
	pvmf_port_base_impl.cpp \
	pvmf_timestamp.cpp \
	pvmf_simple_media_buffer.cpp \
	pvmi_config_and_capability_utils.cpp \
	pvmf_node_interface.cpp \
	pvmf_sync_util.cpp \
	pvmf_basic_errorinfomessage.cpp \
	pvmi_kvp_util.cpp \
	pvmf_mempool.cpp \
	pvmf_duration_infomessage.cpp

HDRS =  pv_interface.h \
	pvmf_node_interface.h \
	pvmi_config_and_capability.h \
	pvmf_event_handling.h \
	pvmf_node_utils.h \
	pvmi_config_and_capability_observer.h \
	pvmf_format_type.h \
	pvmf_pool_buffer_allocator.h \
	pvmf_media_msg_format_ids.h \
	pvmf_media_cmd.h \
	pvmf_port_base_impl.h \
	pvmi_config_and_capability_utils.h \
	pvmf_media_data.h \
	pvmf_port_interface.h \
	pvmi_kvp.h \
	pvmf_media_data_impl.h \
	pvmf_return_codes.h \
	pvmi_media_io_observer.h \
	pvmf_media_frag_group.h \
	pvmf_simple_media_buffer.h \
	pvmi_media_transfer.h \
	pvmf_media_msg.h \
	pvmf_sync_util_data_queue.h \
	pvmi_mio_control.h \
	pvmf_media_msg_header.h \
	pvmf_sync_util.h \
	pvmp4h263encextension.h \
	pvmf_meta_data_types.h \
	pvmf_timestamp.h \
	pv_uuid.h \
	pvmf_node_cmd_msg.h \
	pvmf_video.h \
	pvmf_basic_errorinfomessage.h \
	pvmf_errorinfomessage_extension.h \
	pvmi_media_io_clock_extension.h \
	pvmf_fileformat_events.h \
	pvmi_kvp_util.h \
	pvmf_mempool.h \
	pvmi_port_config_kvp.h \
	pvmi_drm_kvp.h \
	pvmi_data_stream_interface.h \
	pvmi_datastreamuser_interface.h \
	pvmf_resizable_simple_mediamsg.h \
	pvmf_duration_infomessage.h \
	pvmf_durationinfomessage_extension.h
ifeq ($(PACKETSOURCE),1)
HDRS += pvmf_rtp_packet_source.h
endif

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

