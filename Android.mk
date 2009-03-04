ifneq ($(BUILD_WITHOUT_PV),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Set up the PV variables.
include $(LOCAL_PATH)/Config.mk

# Install the default configuration file
# if no value-add configuration is present.
ifneq ($(VALUE_ADD),1)
$(call add-prebuilt-files, ETC, pvplayer.conf)
endif

include $(PV_TOP)/pvcommon/Android.mk
include $(PV_TOP)/pvplayer/Android.mk
include $(PV_TOP)/pvauthor/Android.mk
include $(PV_TOP)//baselibs/gen_data_structures/Android.mk
include $(PV_TOP)//protocols/rtp/Android.mk
include $(PV_TOP)//protocols/sdp/parser/Android.mk
include $(PV_TOP)//protocols/rtp_payload_parser/Android.mk
include $(PV_TOP)//protocols/rtsp_parcom/Android.mk
include $(PV_TOP)//protocols/rtsp_client_engine/Android.mk
include $(PV_TOP)//protocols/http_parcom/Android.mk
include $(PV_TOP)//nodes/pvsocketnode/Android.mk
include $(PV_TOP)//nodes/pvdownloadmanagernode/Android.mk
include $(PV_TOP)//nodes/pvprotocolenginenode/Android.mk
include $(PV_TOP)//nodes/streaming/jitterbuffernode/Android.mk
include $(PV_TOP)//nodes/streaming/medialayernode/Android.mk
include $(PV_TOP)//nodes/streaming/streamingmanager/Android.mk
include $(PV_TOP)//pvmi/recognizer/plugins/pvmp4ffrecognizer/Android.mk
include $(PV_TOP)//codecs_v2/video/m4v_h263/dec/Android.mk
include $(PV_TOP)//nodes/pvmp4ffparsernode/Android.mk
include $(PV_TOP)//codecs_v2/omx/omx_m4v/Android.mk
include $(PV_TOP)//codecs_v2/audio/sbc/enc/Android.mk
#include $(PV_TOP)//engines/player/sample_app/Android.mk
#include $(PV_TOP)//engines/player/test/Android.mk
#include $(PV_TOP)//engines/adapters/player/framemetadatautility/test/Android.mk
#include $(PV_TOP)//engines/author/test/Android.mk
endif
