
# Makefile segment for PDL
#

XCPPFLAGS += -DPV_PROTOCOL_ENGINE_NODE_PROGRESSIVE_DOWNLOAD_ENABLED 

SRCS += pvmf_protocol_engine_progressive_download.cpp \
	pvmf_protocol_engine_node_progressive_download.cpp
