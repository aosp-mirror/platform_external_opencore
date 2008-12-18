ifneq ($(BUILD_WITHOUT_PV),true)
LOCAL_PATH := $(call my-dir)
PV_TOP := $(LOCAL_PATH)

include $(CLEAR_VARS)

PV_CFLAGS := -Wno-non-virtual-dtor -DENABLE_MEMORY_PLAYBACK
include $(CLEAR_VARS)

FORMAT := nj

PV_COPY_HEADERS_TO := libpv

PV_INCLUDES := \
	$(PV_TOP)/android \
	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
	$(PV_TOP)/engines/common/include \
	$(PV_TOP)/engines/player/config/linux_nj \
	$(PV_TOP)/engines/player/include \
	$(PV_TOP)/nodes/pvmediaoutputnode/include \
	$(PV_TOP)/nodes/pvdownloadmanagernode/config/opencore \
	$(PV_TOP)/pvmi/pvmf/include \
	$(PV_TOP)/fileformats/mp4/parser/config/opencore \
	$(PV_TOP)/oscl/oscl/config/linux_nj \
	$(PV_TOP)/oscl/oscl/config/shared \
	$(PV_TOP)/engines/author/include \
	$(PV_TOP)/android/drm/oma1/src \
	$(TARGET_OUT_HEADERS)/$(PV_COPY_HEADERS_TO) 

ALTERNATE_CONFIG := $(if $(wildcard vendor/pv/pvplayer.conf),true)
ifneq ($(ALTERNATE_CONFIG), true)
$(call add-prebuilt-files, ETC, pvplayer.conf)
else
VALUE_ADD := 1
PV_CFLAGS += -DPV_USE_VALUE_ADD=1
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
