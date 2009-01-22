# Get the current local path as the first operation
LOCAL_PATH := $(call get_makefile_dir)


# Clear out the variables used in the local makefiles
include $(MK)/clear.mk

TARGET := protocolenginenode


XCXXFLAGS += $(FLAG_COMPILE_WARNINGS_AS_ERRORS)


XINCDIRS +=  ../../config/$(BUILD_ARCH) ../../config/linux




SRCDIR := ../../src
INCSRCDIR := ../../include

SRCS := pvmf_protocol_engine_common.cpp \
	pvmf_protocol_engine_node_common.cpp \
	pvmf_protocol_engine_factory.cpp \
	pvmf_protocol_engine_port.cpp \

HDRS := pvmf_protocol_engine_factory.h \
	pvmf_protocol_engine_defs.h \
	pvmf_protocol_engine_node_extension.h \
	pvmf_protocol_engine_command_format_ids.h \
	pvmf_protocol_engine_node_events.h


# include $(call process_include_list,$(LOCAL_PATH),$(PROTOCOL_PLUGINS))

include $(MK)/library.mk
