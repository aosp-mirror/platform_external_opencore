include $(PV_TOP)/makefile.pv

WHAT = module
TARGET = pvplayer

export FORMAT=nj
LIBDIRS = \
    $(VOB_BASE_DIR)/engines/player/build/make \
    $(VOB_BASE_DIR)/codecs_v2/audio/aac/dec/util/getactualaacconfig/build/make \
    $(VOB_BASE_DIR)/codecs_v2/video/avc_h264/dec/build/make \
    $(VOB_BASE_DIR)/codecs_v2/audio/aac/dec/build/make \
    $(VOB_BASE_DIR)/codecs_v2/audio/gsm_amr/amr_nb/dec/build/make \
    $(VOB_BASE_DIR)/codecs_v2/audio/gsm_amr/amr_wb/dec/build/make \
    $(VOB_BASE_DIR)/codecs_v2/audio/gsm_amr/common/dec/build/make \
    $(VOB_BASE_DIR)/codecs_v2/audio/mp3/dec/build/make \
    $(VOB_BASE_DIR)/codecs_v2/utilities/m4v_config_parser/build/make \
    $(VOB_BASE_DIR)/codecs_v2/utilities/pv_video_config_parser/build/make \
    $(VOB_BASE_DIR)/codecs_v2/omx/omx_common/build/make_multithreaded \
    $(VOB_BASE_DIR)/codecs_v2/omx/omx_queue/build/make \
    $(VOB_BASE_DIR)/codecs_v2/omx/omx_h264/build/make_multithreaded \
    $(VOB_BASE_DIR)/codecs_v2/omx/omx_aac/build/make_multithreaded \
    $(VOB_BASE_DIR)/codecs_v2/omx/omx_amr/build/make_multithreaded \
    $(VOB_BASE_DIR)/codecs_v2/omx/omx_mp3/build/make_multithreaded \
    $(VOB_BASE_DIR)/codecs_v2/omx/factories/omx_m4v_factory/build/make \
    $(VOB_BASE_DIR)/codecs_v2/omx/omx_proxy/build/make \
    $(VOB_BASE_DIR)/baselibs/gen_data_structures/bitstreamparser/build/make \
    $(VOB_BASE_DIR)/nodes/common/build/make \
    $(VOB_BASE_DIR)/pvmi/content_policy_manager/build/make \
    $(VOB_BASE_DIR)/pvmi/content_policy_manager/plugins/oma1/passthru/build/make \
    $(VOB_BASE_DIR)/pvmi/content_policy_manager/plugins/common/build/make \
    $(VOB_BASE_DIR)/pvmi/media_io/pvmiofileoutput/build/make \
    $(VOB_BASE_DIR)/fileformats/common/parser/build/make \
    $(VOB_BASE_DIR)/fileformats/id3parcom/build/make \
    $(VOB_BASE_DIR)/fileformats/rawgsmamr/parser/build/make \
    $(VOB_BASE_DIR)/fileformats/mp3/parser/build/make \
    $(VOB_BASE_DIR)/fileformats/mp4/parser/build_opencore/make \
    $(VOB_BASE_DIR)/fileformats/rawaac/parser/build/make \
    $(VOB_BASE_DIR)/fileformats/wav/parser/build/make \
    $(VOB_BASE_DIR)/nodes/pvaacffparsernode/build/make \
    $(VOB_BASE_DIR)/nodes/pvmp3ffparsernode/build/make \
    $(VOB_BASE_DIR)/nodes/pvamrffparsernode/build/make \
    $(VOB_BASE_DIR)/nodes/pvmediaoutputnode/build/make \
    $(VOB_BASE_DIR)/nodes/pvomxvideodecnode/build/make \
    $(VOB_BASE_DIR)/nodes/pvomxaudiodecnode/build/make \
    $(VOB_BASE_DIR)/nodes/pvwavffparsernode/build/make \
    $(VOB_BASE_DIR)/pvmi/recognizer/build/make \
    $(VOB_BASE_DIR)/pvmi/recognizer/plugins/pvamrffrecognizer/build/make \
    $(VOB_BASE_DIR)/pvmi/recognizer/plugins/pvmp3ffrecognizer/build/make \
    $(VOB_BASE_DIR)/pvmi/recognizer/plugins/pvwavffrecognizer/build/make \
    $(VOB_BASE_DIR)/pvmi/recognizer/build/make \
    $(VOB_BASE_DIR)/engines/common/build/make \
    $(VOB_BASE_DIR)/engines/adapters/player/framemetadatautility/build/make \
    $(VOB_BASE_DIR)/protocols/rtp_payload_parser/util/build/latmparser/make

LIBS = -lgetactualaacconfig \
       -lpvavcdecoder -lpv_aac_dec -lpvdecoder_gsmamr \
       -lpvamrwbdecoder -lpvmp3 \
       -lm4v_config -lpv_video_config_parser \
       -lomx_common_lib -lomx_queue_lib -lomx_avc_component_lib \
       -lomx_m4v_component_interface -lpvomx_proxy_lib \
       -lomx_aac_component_lib -lomx_amr_component_lib -lomx_mp3_component_lib \
       -lcpm -lpassthru_oma1 -lfile_mediaio -lpvid3parcom \
       -lpvgsmamrparser -lpvmp3ff -lpvmp4ff -lpvaacparser -lpvwav -lpvfileparserutils \
       -lpvaacffparsernode -lpvmp3ffparsernode \
       -lpvamrffparsernode \
       -lpvmediaoutputnode \
       -lpvomxvideodecnode -lpvomxaudiodecnode -lpvwavffparsernode \
       -lpvmfrecognizer -lpvamrffrecognizer -lpvmp3ffrecognizer \
       -lpvwavffrecognizer \
       -lpvframemetadatautility -lpvplayer_engine -lpvlatmpayloadparser

# use --allow-multiple-definition to get around aac decoder and
# getactualaacconfig building the same code
XLDFLAGS += -Wl,--allow-multiple-definition

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

