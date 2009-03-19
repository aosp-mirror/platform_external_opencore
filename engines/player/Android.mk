LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pv_player_datapath.cpp \
	src/pv_player_engine.cpp \
	src/pv_player_factory.cpp \
	config/linux_nj/pv_player_node_registry.cpp

LOCAL_MODULE := libpvplayer_engine

LOCAL_CFLAGS :=   $(PV_CFLAGS)

LOCAL_C_INCLUDES := \
	$(PV_TOP)//engines/player/include \
	$(PV_TOP)//engines/player/src \
	$(PV_TOP)//engines/player/../common/include \
	$(PV_TOP)/baselibs/media_data_structures/src \
	$(PV_TOP)/nodes/pvaudioplayernode/include \
	$(PV_TOP)/codecs_v2/audio/aac_mpeg4/AAC_baseline/pv_aac_dec/cpp/include/ \
	$(PV_TOP)/codecs_v2/audio/aac_mpeg4/AAC_baseline/pv_aac_dec/c/include/ \
	$(PV_TOP)/codecs_v2/audio/mp3/thompson/include \
	$(PV_TOP)/nodes/pvmediaoutputnode/src \
	$(PV_TOP)/nodes/common/include \
	$(PV_TOP)/pvmi/pvmf/include \
	$(PV_TOP)/nodes/pvwmadecnode/include \
	$(PV_TOP)/nodes/pvwmvdecnode/include \
	$(PV_TOP)/nodes/pvasfffparsernode/include \
	$(PV_TOP)/codecs_v2/wmv_decoder/include \
	$(PV_TOP)/codecs_v2/wma_decoder/include \
	$(PV_TOP)/fileformats/asf/parser/include \
	$(PV_TOP)/pvmi/recognizer/include \
	$(PV_TOP)/pvmi/recognizer/plugins/pvasfffrecognizer/include \
	$(PV_TOP)/pvmi/recognizer/plugins/pvmp4ffrecognizer/include \
	$(PV_TOP)/pvmi/recognizer/plugins/pvmp3ffrecognizer/include \
	$(PV_INCLUDES) 


ifeq ($(PV_OSCL_LIB), false)
    LOCAL_C_INCLUDES += $(PV_TOP)/nodes/streaming/streamingmanager/include
endif


ifeq ($(ARCHITECTURE),linux_nj)
    LOCAL_C_INCLUDES += $(PV_TOP)//engines/player/config/linux_nj
else
	ifeq ($(FORMAT),nj)
	LOCAL_C_INCLUDES += $(PV_TOP)//engines/player/config/linux_nj
	else
	LOCAL_C_INCLUDES += $(PV_TOP)//engines/player/config/linux_nj
	endif
endif

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pv_player_datasinkfilename.h \
	include/pv_player_datasource.h \
	include/pv_player_events.h \
	include/pv_player_factory.h \
	include/pv_player_datasink.h \
	include/pv_player_datasourcepvmfnode.h \
	include/pv_player_interface.h \
	include/pv_player_datasinkpvmfnode.h \
	include/pv_player_datasourceurl.h \
	include/pv_player_types.h \
	include/pv_player_license_acquisition_interface.h \
	include/pv_player_track_selection_interface.h \
	include/pv_player_registry_interface.h

include $(BUILD_STATIC_LIBRARY)

