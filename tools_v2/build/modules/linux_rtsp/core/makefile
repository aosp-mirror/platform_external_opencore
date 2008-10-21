# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = module
TARGET = pvrtsp

LIBDIRS = $(VOB_BASE_DIR)/tools_v2/build/modules/linux_rtsp/core/src \
		  $(VOB_BASE_DIR)/nodes/streaming/streamingmanager/build/linux_3gpp \
		  $(VOB_BASE_DIR)/protocols/rtsp_parcom/build/make \
		  $(VOB_BASE_DIR)/protocols/rtsp_client_engine/build/make_3gpp \
		  $(VOB_BASE_DIR)/protocols/rtp_payload_parser/build/linux_3gpp \
		  $(VOB_BASE_DIR)/protocols/rtp/build/make \
		  $(VOB_BASE_DIR)/nodes/streaming/jitterbuffernode/build/make \
		  $(VOB_BASE_DIR)/nodes/streaming/medialayernode/build/make

LIBS = -lpvrtspinterface \
	   -lpvstreamingmanagernode_3gpp \
	   -lpv_rtsp_parcom \
	   -lpvrtsp_cli_eng_node_3gpp \
	   -lrtppayloadparser_3gpp \
	   -lrtprtcp \
	   -lpvjitterbuffernode \
	   -lpvmediaplayernode

MODS = -lpvnet_support -lpvcommon -lpvplayer

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

