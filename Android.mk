ifneq ($(BUILD_WITHOUT_PV),true)
LOCAL_PATH := $(call my-dir)
#PV_TOP := $(LOCAL_PATH)
include $(CLEAR_VARS)

# Set up the PV variables.
include $(LOCAL_PATH)/Config.mk

# Install the default configuration file
# if no value-add configuration is present.
ifneq ($(VALUE_ADD),1)
$(call add-prebuilt-files, ETC, pvplayer.cfg)
endif


include $(PV_TOP)/build_config/opencore_dynamic/Android_opencore_common.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_opencore_author.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_opencore_player.mk
#include $(PV_TOP)/build_config/opencore_dynamic/Android_opencore_2way.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_omx_sharedlibrary.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_omx_avcdec_sharedlibrary.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_omx_m4vdec_sharedlibrary.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_omx_aacdec_sharedlibrary.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_omx_amrdec_sharedlibrary.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_omx_mp3dec_sharedlibrary.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_omx_avcenc_sharedlibrary.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_omx_m4venc_sharedlibrary.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_omx_amrenc_sharedlibrary.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_opencore_net_support.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_opencore_downloadreg.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_opencore_download.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_opencore_rtspreg.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_opencore_rtsp.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_opencore_mp4localreg.mk
include $(PV_TOP)/build_config/opencore_dynamic/Android_opencore_mp4local.mk
ifeq ($(BUILD_PV_TEST_APPS),1)
include $(PV_TOP)/oscl/unit_test/Android.mk
include $(PV_TOP)/engines/player/test/Android.mk
include $(PV_TOP)/engines/author/test/Android.mk
include $(PV_TOP)/engines/2way/test/Android.mk
endif

endif
