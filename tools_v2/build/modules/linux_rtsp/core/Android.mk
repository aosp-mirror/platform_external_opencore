LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := src/pvmfrtspnodes.cpp

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libpvstreamingmanagernode_3gpp \
	libpv_rtsp_parcom \
	libpvrtsp_cli_eng_node_3gpp \
	librtppayloadparser_3gpp \
	librtprtcp \
	libpvjitterbuffernode \
	libpvmediaplayernode

LOCAL_SHARED_LIBRARIES := liblog libopencorenet_support libopencoreplayer libopencorecommon

LOCAL_MODULE := libopencorertsp

LOCAL_C_INCLUDES := \
	$(PV_INCLUDES)

LOCAL_CFLAGS := $(PV_CFLAGS)

include $(BUILD_SHARED_LIBRARY)
