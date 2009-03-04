LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvmp4ffcn_port.cpp \
	src/pvmp4ffcn_node.cpp \
	src/pvmp4ffcn_node_cap_config.cpp



LOCAL_MODULE := libpvmp4ffcomposernode

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)/fileformats/mp4/composer/include \
	$(PV_TOP)//nodes/pvmp4ffcomposernode/include \
	$(PV_TOP)//nodes/pvmp4ffcomposernode/src \
	$(PV_TOP)/$(SDK_LOCAL)/installed_include \
	$(PV_TOP)//nodes/pvmp4ffcomposernode/src \
	$(PV_TOP)//nodes/pvmp4ffcomposernode/../../nodes/common/include \
	$(PV_TOP)//nodes/pvmp4ffcomposernode/../../pvmi/pvmf/include \
	$(PV_TOP)//nodes/pvmp4ffcomposernode/include \
	$(PV_TOP)//nodes/pvmp4ffcomposernode/../../baselibs/pv_mime_utils/src \
	$(PV_TOP)//nodes/pvmp4ffcomposernode/../../engines/author/include \
	$(PV_TOP)//nodes/pvmp4ffcomposernode/../../baselibs/media_data_structures/src \
	$(PV_TOP)//nodes/pvmp4ffcomposernode/../../oscl/oscl/osclio/src \
	$(PV_TOP)//nodes/pvmp4ffcomposernode/../../fileformats/mp4/composer/config/opencore \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvmp4ffcn_clipconfig.h \
	include/pvmp4ffcn_factory.h \
	include/pvmp4ffcn_trackconfig.h \
	include/pvmp4ffcn_types.h

include $(BUILD_STATIC_LIBRARY)

