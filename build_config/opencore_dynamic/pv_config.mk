#
# Automatically generated, don't edit
#
# At: Fri, 16 Jan 2009 06:11:38 +0000

#
# PV Code Base Configuration System
#

#
# Menu for selecting supported features
#
module_support=y


#
# Menu for configuring runtime loadable modules
#
pv_so=n
pvsplit_so=y
streaming_support=y

#
# Menu for selecting streaming features
#
rtsp_support=y
asf_streaming_support=n

download_support=y
mp4local_support=y
asflocal_support=n
janus_support=n
wmdrmplat_support=n
wmdrmdev_support=n
mtp_db_support=n


#
# Menu for configuring Baselibs
#
csprng_lib=n
pvcrypto_lib=n
pventropysrc_lib=n
pvgendatastruct_lib=m
pvmediadatastruct_lib=m
pvmimeutils_lib=m
threadsafe_callback_ao_lib=m
pvthreadmessaging_lib=m


#
# Menu for configuring File Formats
#
pvasfff_lib=n
pvmp3ff_lib=m
pvmp4ffcomposer_lib=n
pvmp4ffcomposeropencore_lib=m
pvmp4ff_lib=n
pvmp4ffopencore_lib=m
mp4recognizer_utility_lib=m
pvaacparser_lib=m
pvgsmamrparser_lib=m
pvrmff_lib=n
pvrmffparser_lib=n
pvfileparserutils_lib=m
pvid3parcom_lib=m
pvpvxparser_lib=n
pvwav_lib=m
pvasxparser_lib=n
pvavifileparser_lib=m
pvpvrff_lib=n
asfrecognizer_utility_lib=n
pv_divxfile_parser_lib=n


#
# Menu for configuring Codecs
#

#
# Menu for configuring OMX Support
#
omx_mastercore_lib=m
MAX_NUMBER_OF_OMX_CORES=10
MAX_NUMBER_OF_OMX_COMPONENTS=50
pv_omx=y
omx_avc_component_lib=m
omx_common_lib=m
omx_m4v_component_lib=m
omx_queue_lib=m
omx_wmv_component_lib=n
pvomx_proxy_lib=m
omx_aac_component_lib=m
omx_amr_component_lib=m
omx_mp3_component_lib=m
omx_wma_component_lib=n
omx_amrenc_component_lib=m
omx_m4venc_component_lib=m
omx_avcenc_component_lib=m
omx_baseclass_lib=m


#
# Menu for configuring audio codecs
#
pv_aac_dec_lib=m
getactualaacconfig_lib=m
pv_amr_nb_common_lib=m
pvdecoder_gsmamr_lib=m
pvencoder_gsmamr_lib=m
pvamrwbdecoder_lib=m
gsm_amr_headers_lib=m
pvmp3_lib=m
pvra8decoder_lib=n
wmadecoder_lib=n


#
# Menu for configuring video codecs
#
pv_avc_common_lib=m
pvavcdecoder_lib=m
wmvdecoder_lib=n
pvmp4decoder_lib=m
rvdecoder_lib=n
pvm4vencoder_lib=m
pvavch264enc_lib=m


#
# Menu for configuring codecs utilities
#
m4v_config_lib=m
pv_config_parser_lib=m
colorconvert_lib=m



#
# Menu for configuring Nodes
#

#
# Menu for configuring Streaming
#
pvstreamingmanagernode_segments_lib=m

#
# Menu for configuring Streaming Features
#
mshttp_support=n
rtspunicast_support=y
unicastpvr_support=n
broadcastpvr_support=n
pvrfileplayback_support=n

pvstreamingmanagernode_lib=n
pvstreamingmanagernode_3gpp_lib=n
pvmedialayernode_lib=m
pvmedialayernode_opencore_lib=n
pvjitterbuffernode_lib=m
pvjitterbuffernode_opencore_lib=n


#
# Menu for configuring Download
#
pvdownloadmanagernode_lib=m

#
# Menu for configuring downloadmanager features
#
PVMF_DOWNLOADMANAGER_SUPPORT_PVX=n
PVMF_DOWNLOADMANAGER_SUPPORT_PPB=y
PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE=n
PVMF_DOWNLOADMANAGER_MIN_TCP_BUFFERS_FOR_PPB=13
ENABLE_LARGE_MBDS_CACHE_SIZE=n



#
# Menu for configuring ProtocolEngine
#
pvprotocolenginenode_segments_lib=m
pvprotocolenginenode_lib=n

#
# Menu for configuring ProtocolEngine Features
#
ps_support=y
pdl_support=y
ftdl_support=n
http_support=n


pvfileoutputnode_lib=m
pvmediaoutputnode_lib=m
pvsocketnode_lib=m
pvavcdecnode_lib=n
pvvideodecnode_lib=n
pvwavffparsernode_lib=m
pvwmadecnode_lib=n
pvwmvdecnode_lib=n
pvomxencnode_lib=m
pvomxaudiodecnode_lib=m
pvomxbasedecnode_lib=m
pvomxvideodecnode_lib=m
aacdecnode_lib=n
pvaacffparsernode_lib=m
gsmamrdecnode_lib=n
pvamrffparsernode_lib=m
pvasfffparsernode_lib=n
mp3decnode_lib=n
pvmp3ffparsernode_lib=m
pvmp4ffparsernode_lib=n
pvmp4ffparsernodeopencore_lib=m
pvrvdecnode_lib=n
pvra8decnode_lib=n
pvrmffparsernode_lib=n
pvrtppacketsourcenode_lib=n
nodes_common_headers_lib=m
pvamrencnode_lib=n
pvmediainputnode_lib=m
pvmp4ffcomposernode_lib=n
pvmp4ffcomposernodeopencore_lib=m
pvvideoencnode_lib=n
pvavcencnode_lib=n
pvpvr_lib=n
pvpvrnode_lib=n
pvcommsionode_lib=m
pvclientserversocketnode_lib=m
pvloopbacknode_lib=m
pvvideoparsernode_lib=m
pvdummyinputnode_lib=m
pvdummyoutputnode_lib=m
pvdivxffparsernode_lib=n


#
# Menu for configuring Oscl
#
build_oscl=m
unit_test_lib=y


#
# Menu for configuring Protocols
#

#
# Menu for configuring Value Adds for 2way
#
twoway_value_add_config=y
PV_2WAY_VALUE_ADD_NONE=y

pv_http_parcom_lib=m
pvlatmpayloadparser_lib=m
sdp_common=m

#
# Menu for configuring SDPParser
#
sdp_default=n
sdp_opencore=m

rdt_parser_lib=n
pv_rtsp_parcom_lib=m
pvrtsp_cli_eng_node_lib=n
pvrtsp_cli_eng_node_3gpp_lib=n
pvrtsp_cli_eng_node_opencore_lib=m
rtppayloadparser_lib=m

#
# Menu for rtppayload parser plugins
#
rfc_2429=y
rfc_3016=y
rfc_3267=y
rfc_3640=y
rfc_3984=y
asf_payload=n
realmedia_payload=n

rtprtcp_lib=m
pv324m_lib=m
pv324m_common_headers_lib=m
pvgeneraltools_lib=m


#
# Menu for configuring Pvmi
#

#
# Menu for configuring Recognizers
#
pvmfrecognizer_lib=m
pvaacffrecognizer_lib=m
pvamrffrecognizer_lib=m
pvoma1ffrecognizer_lib=n
pvasfffrecognizer_lib=n
pvmp3ffrecognizer_lib=m
pvmp4ffrecognizer_lib=m
pvwavffrecognizer_lib=m
pvrmffrecognizer_lib=n
pvdivxffrecognizer_lib=n


#
# Menu for configuring Content Policy Manager
#
cpm_lib=m
passthru_oma1_lib=m
pvjanusplugin_lib=n
cpm_headers_lib=m
pvoma1lockstream_lib=n


#
# Menu for configuring Media IO
#
pvmiofileinput_lib=m
pvmiofileoutput_lib=m
pvmioaviwavfileinput_lib=m
pvmio_comm_loopback_lib=m


#
# Menu for configuring PacketSources
#
packetsources_default_lib=n

#
# Menu for configuring PacketSource Plugins
#
optimized_bcast_ps_support=n
standard_bcast_ps_support=n


pvmf_lib=m
realaudio_deinterleaver_lib=n
pvdbmanager_lib=n


#
# Menu for configuring Engines
#

#
# Menu for configuring Player
#
pvplayer_engine_lib=m

#
# Menu for player engine tunables
#
PVPLAYERENGINE_CONFIG_SKIPTOREQUESTEDPOS_DEF=y
PVPLAYERENGINE_CONFIG_SYNCMARGIN_EARLY_DEF=-200
PVPLAYERENGINE_CONFIG_SYNCMARGIN_LATE_DEF=200
VIDEO_DEC_NODE_LOW_PRIORITY=y


#
# Menu for configuring player registry
#
BUILD_OMX_VIDEO_DEC_NODE=y
BUILD_OMX_AUDIO_DEC_NODE=y
BUILD_VIDEO_DEC_NODE=n
BUILD_AVC_DEC_NODE=n
BUILD_WMV_DEC_NODE=n
BUILD_RV_DEC_NODE=n
BUILD_WMA_DEC_NODE=n
BUILD_G726_DEC_NODE=n
BUILD_GSMAMR_DEC_NODE=n
BUILD_AAC_DEC_NODE=n
BUILD_MP3_DEC_NODE=n
BUILD_RA8_DEC_NODE=n
BUILD_MP4_FF_PARSER_NODE=n
BUILD_AMR_FF_PARSER_NODE=y
BUILD_AAC_FF_PARSER_NODE=y
BUILD_MP3_FF_PARSER_NODE=y
BUILD_WAV_FF_PARSER_NODE=y
BUILD_ASF_FF_PARSER_NODE=n
BUILD_RM_FF_PARSER_NODE=n
BUILD_STREAMING_MANAGER_NODE=n
BUILD_DOWNLOAD_MANAGER_NODE=n
BUILD_STILL_IMAGE_NODE=n
BUILD_MP4_FF_REC=n
BUILD_ASF_FF_REC=n
BUILD_OMA1_FF_REC=n
BUILD_AAC_FF_REC=y
BUILD_RM_FF_REC=n
BUILD_MP3_FF_REC=y
BUILD_WAV_FF_REC=y
BUILD_AMR_FF_REC=y



#
# Menu for configuring Author
#
pvauthorengine_lib=m


#
# Menu for configuring pv2way
#
pv2wayengine_lib=m

engines_common_headers_lib=m
pvframemetadatautility_lib=m


#
# Menu for configuring Extern_libs
#
pvmtp_engine_lib=n
pvsqlite_lib=n
pvwmdrm_lib=n
wmdrm_config=n


#
# Derived symbols
#
pvmediaoutputnode_y_mk=""
LIBS_omxenc_shared="-lomx_amrenc_component_lib -lomx_m4venc_component_lib -lomx_avcenc_component_lib"
pvavcencnode_y_lib=""
pventropysrc_y_lib=""
pvjitterbuffernode_y_mk=""
pvvideoencnode_y_mk=""
pvamrwbdecoder_imp_m_mk=""
protocolenginenode_segments_m_lib="-lprotocolenginenode"
aacdecnode_m_mk=""
pvclientserversocketnode_y_lib=""
pvencoder_gsmamr_y_lib=""
pvstreamingmanagernode_m_lib=""
pvmedialayernode_m_mk="/nodes/streaming/medialayernode/build/make"
pvpvr_y_mk=""
pvmp4ffcomposernode_m_lib=""
pvwmdrmplatinterface_m_lib=""
omx_amrenc_component_m_mk="/codecs_v2/omx/omx_amrenc/build/make_multithreaded"
sdp_common_m_mk="/protocols/sdp/common/build/make"
sdp_parser_mksegment_opencore="sdp_opencore.mk"
pvloopbacknode_y_mk=""
pvrtspreginterface_m_mk="/modules/linux_rtsp/node_registry/build/make"
LIBDIR_oscl_static=" /oscl/unit_test/build/make"
SOLIBDIRS_omx_m4venc_sharedlibrary="/codecs_v2/omx/omx_m4venc/build/make_multithreaded /codecs_v2/video/m4v_h263/enc/build/make"
pvrmff_m_lib=""
pvjitterbuffernode_plugins_pvasfstreaming="jb_asf.mk"
omx_wmv_component_y_lib=""
pvwmdrm_m_mk=""
omx_amrenc_component_m_lib="-lomx_amrenc_component_lib"
pvdummyinputnode_y_mk=""
pvfileparserutils_y_mk=""
pvrmffparser_m_lib=""
pvm4vencoder_m_mk="/codecs_v2/video/m4v_h263/enc/build/make"
LIBDIR_omxdecimp_shared=""
pv_aac_dec_y_mk=""
DYNAMIC_LOAD_OMX_WMV_COMPONENT=0
SOLIBS_omx_avcdec_sharedlibrary="-lomx_avc_component_lib -lpvavcdecoder"
MODS_omx_aacdec_sharedlibrary="-lomx_sharedlibrary -lopencore_common"
rvdecoder_m_lib=""
pvmimeutils_y_lib=""
pvavch264enc_m_lib="-lpvavch264enc"
pvmf_m_mk="/pvmi/pvmf/build/make"
pv2wayengine_m_lib="-lpv2wayengine"
pvrtspreginterface_m_lib="-lpvrtspreginterface"
pvomxaudiodecnode_y_mk=""
DYNAMIC_LOAD_OMX_AAC_COMPONENT=1
SOLIBS_omx_wmvdec_sharedlibrary=" "
pvamrffrecognizer_m_lib="-lpvamrffrecognizer"
pvgsmamrparser_m_lib="-lpvgsmamrparser"
LIBS_codecs_v2_static="                                   "
omx_wmv_component_m_mk=""
DYNAMIC_LOAD_OMX_AMR_COMPONENT=1
pvmp4ffcomposernode_m_mk=""
pv_divxfile_parser_m_lib=""
omx_mp3_component_m_lib="-lomx_mp3_component_lib"
pvaacparser_m_lib="-lpvaacparser"
mp4recognizer_utility_m_lib="-lmp4recognizer_utility"
SOLIBDIRS_pvjanus=" "
USING_OMX=1
pvmediainputnode_m_lib="-lpvmediainputnode"
protocolenginenode_segments_m_mk="/nodes/pvprotocolenginenode/build/make_segments"
asfrecognizer_utility_m_lib=""
pvmp4ff_m_mk=""
pvdecoder_gsmamr_y_mk=""
pvavifileparser_m_lib="-lpvavifileparser"
LIBS_omxencimp_static=""
LIBS_codecs_v2_shared="-lomx_avc_component_lib -lomx_m4v_component_lib  -lomx_aac_component_lib -lomx_amr_component_lib -lomx_mp3_component_lib  -lomx_amrenc_component_lib -lomx_m4venc_component_lib -lomx_avcenc_component_lib -lomx_common_lib -lomx_queue_lib -lpvomx_proxy_lib -lomx_baseclass_lib -lomx_mastercore_lib -lpv_omx_interface   -lpv_aac_dec -lpv_amr_nb_common_lib -lpvamrwbdecoder -lpvdecoder_gsmamr -lpvmp3  -lpvencoder_gsmamr -lpv_avc_common_lib -lpvavcdecoder  -lpvmp4decoder  -lpvm4vencoder -lpvavch264enc -lm4v_config -lpv_config_parser -lcolorconvert"
omx_avcenc_component_m_lib="-lomx_avcenc_component_lib"
pvavifileparser_y_mk=""
MODS_opencore_2way="-lopencore_common"
omx_m4v_component_y_lib=""
omx_baseclass_y_lib=""
LIBS_packetsources_static="n"
pvdecoder_gsmamr_imp_m_lib=""
LIBDIR_fileformats_shared="/fileformats/common/parser/build/make /fileformats/id3parcom/build/make  /fileformats/wav/parser/build/make  /fileformats/avi/parser/build/make  /fileformats/mp3/parser/build/make /fileformats/rawaac/parser/build/make /fileformats/rawgsmamr/parser/build/make    /fileformats/mp4/parser/utils/mp4recognizer/build/make /fileformats/mp4/parser/build_opencore/make  /fileformats/mp4/composer/build_opencore/make   "
pvdummyoutputnode_m_mk="/nodes/pvdummyoutputnode/build/make"
pvasfff_m_lib=""
pvrmff_m_mk=""
csprng_m_lib=""
pv_avc_common_lib_y_mk=""
pvavcdecoder_m_mk="/codecs_v2/video/avc_h264/dec/build/make"
SOLIBDIRS_pvasfstreaming=" /nodes/streaming/streamingmanager/build/make_segments /nodes/streaming/jitterbuffernode/build/make /nodes/streaming/medialayernode/build/make /protocols/rtp_payload_parser/build/make /protocols/rtp/build/make /nodes/pvprotocolenginenode/build/make_segments"
LIBDIR_extern_libs_shared="    "
protocolenginenode_y_mk=""
omx_mastercore_m_mk="/codecs_v2/omx/omx_mastercore/build/make_multithreaded"
m4v_config_m_mk="/codecs_v2/utilities/m4v_config_parser/build/make"
pvcrypto_y_lib=""
LIBDIR_packetsources_shared="n"
pvfileparserutils_y_lib=""
pvasfcommon_so_name=""
LIBDIR_omxjoint_static="   "
LIBDIR_video_static="      "
wmadecoder_m_lib=""
pv_amr_nb_common_lib_y_lib=""
pventropysrc_m_mk=""
asfrecognizer_utility_y_mk=""
pv_amr_nb_common_lib_m_lib="-lpv_amr_nb_common_lib"
pv_aac_dec_plugins="aacdec_util.mk"
wmvdecoder_y_mk=""
pvmp3ffrecognizer_y_mk=""
REGISTER_OMX_H263ENC_COMPONENT=1
LIBDIR_codecs_v2_shared="/codecs_v2/omx/omx_h264/build/make_multithreaded /codecs_v2/omx/omx_m4v/build/make_multithreaded  /codecs_v2/omx/omx_aac/build/make_multithreaded /codecs_v2/omx/omx_amr/build/make_multithreaded /codecs_v2/omx/omx_mp3/build/make_multithreaded  /codecs_v2/omx/omx_amrenc/build/make_multithreaded /codecs_v2/omx/omx_m4venc/build/make_multithreaded /codecs_v2/omx/omx_h264enc/build/make_multithreaded /codecs_v2/omx/omx_common/build/make_multithreaded /codecs_v2/omx/omx_queue/build/make /codecs_v2/omx/omx_proxy/build/make /codecs_v2/omx/omx_baseclass/build/make /codecs_v2/omx/omx_mastercore/build/make_multithreaded /codecs_v2/omx/omx_sharedlibrary/interface/build/make   /codecs_v2/audio/aac/dec/build/make /codecs_v2/audio/gsm_amr/amr_nb/common/build/make /codecs_v2/audio/gsm_amr/amr_wb/dec/build/make /codecs_v2/audio/gsm_amr/amr_nb/dec/build/make /codecs_v2/audio/mp3/dec/build/make  /codecs_v2/audio/gsm_amr/common/dec/build/make /codecs_v2/audio/gsm_amr/amr_nb/enc/build/make /codecs_v2/video/avc_h264/common/build/make /codecs_v2/video/avc_h264/dec/build/make  /codecs_v2/video/m4v_h263/dec/build/make  /codecs_v2/video/m4v_h263/enc/build/make /codecs_v2/video/avc_h264/enc/build/make /codecs_v2/utilities/m4v_config_parser/build/make /codecs_v2/utilities/pv_config_parser/build/make /codecs_v2/utilities/colorconvert/build/make"
omx_amrenc_component_imp_m_mk=""
omx_avc_component_m_mk="/codecs_v2/omx/omx_h264/build/make_multithreaded"
pvdecoder_gsmamr_m_lib="-lpvdecoder_gsmamr"
pvmf_y_mk=""
omx_mastercore_m_lib="-lomx_mastercore_lib"
getactualaacconfig_imp_m_lib="-lgetactualaacconfig"
pvfileoutputnode_y_mk=""
LIBDIR_recognizer_static="         "
SOLIBS_pvwmdrmdev="n"
realaudio_deinterleaver_y_lib=""
cpm_m_lib="-lcpm"
pvmp4ffcomposeropencore_m_lib="-lpvmp4ffcomposer"
LIBDIR_engines_shared="/engines/player/build/make /engines/author/build/make /engines/2way/build/make /engines/common/build/make /engines/adapters/player/framemetadatautility/build/make"
omx_amr_component_imp_m_lib=""
pvdummyoutputnode_m_lib="-lpvdummyoutputnode"
pvmp4ffcomposeropencore_m_mk="/fileformats/mp4/composer/build_opencore/make"
SOLIBS_opencore_author=" -lpvmp4ffcomposer  -lpvmp4ffcomposernode -lpvauthorengine"
pvmp3ff_y_lib=""
pv324m_plugins="default_support.mk"
MODS_omx_wmadec_sharedlibrary="-lomx_sharedlibrary -lopencore_common"
omx_m4vdec_sharedlibrary_so_name="omx_m4vdec_sharedlibrary"
pvrtppacketsourcenode_y_mk=""
pvmiofileinput_y_lib=""
pvavcdecoder_m_lib="-lpvavcdecoder"
LIBS_fileformats_static="                  "
SOLIBDIRS_opencore_mp4localreg="/modules/linux_mp4/node_registry/build/make /pvmi/recognizer/plugins/pvmp4ffrecognizer/build/make"
pvamrffrecognizer_y_mk=""
REGISTER_OMX_WMV_COMPONENT=0
omx_m4v_component_m_lib="-lomx_m4v_component_lib"
pvmp3ffparsernode_y_mk=""
pventropysrc_m_lib=""
pvsdpparser_y_lib=""
SOLIBDIRS_opencore_author=" /fileformats/mp4/composer/build_opencore/make  /nodes/pvmp4ffcomposernode/build_opencore/make /engines/author/build/make"
packetsources_default_y_lib=""
LIBDIR_baselibs_shared="   /baselibs/gen_data_structures/build/make /baselibs/media_data_structures/build/make /baselibs/pv_mime_utils/build/make /baselibs/threadsafe_callback_ao/build/make /baselibs/thread_messaging/build/make"
pvomx_proxy_m_mk="/codecs_v2/omx/omx_proxy/build/make"
LIBS_recognizer_shared="-lpvmfrecognizer -lpvaacffrecognizer -lpvamrffrecognizer   -lpvmp3ffrecognizer -lpvmp4ffrecognizer -lpvwavffrecognizer  "
pvasf_streaming_so_name=""
pvsocketnode_m_mk="/nodes/pvsocketnode/build/make"
pvstreamingmanagernode_3gpp_m_mk=""
pv2wayengine_y_lib=""
pvwmadecnode_y_mk=""
pvaacffrecognizer_y_lib=""
pvpvrff_m_mk=""
pvomxvideodecnode_y_lib=""
LIBS_omxencimp_shared=""
pvdownloadmanagernode_m_lib="-lpvdownloadmanagernode"
pvdivxffrecognizer_m_lib=""
pvmp4ffrecognizer_m_lib="-lpvmp4ffrecognizer"
opencore_player_so_name="opencore_player"
pvvideoparsernode_y_mk=""
pvmp4ffcomposer_m_lib=""
dl_common_mk="pe_dl_common.mk"
pvjitterbuffernode_opencore_m_mk=""
pvmp4ffcomposer_y_lib=""
threadsafe_callback_ao_m_lib="-lthreadsafe_callback_ao"
pvmp4ffopencore_m_lib="-lpvmp4ff"
pvencoder_gsmamr_m_mk="/codecs_v2/audio/gsm_amr/amr_nb/enc/build/make"
sdp_common_y_mk=""
pvdecoder_gsmamr_m_mk="/codecs_v2/audio/gsm_amr/amr_nb/dec/build/make"
MODS_pvwmdrmdev="-lopencore_player -lopencore_common"
pvmio_comm_loopback_m_lib="-lpvmio_comm_loopback"
pvrtppacketsourcenode_y_lib=""
pvasfstreaminginterface_m_mk=""
pvwmdrmdev_so_name=""
rvdecoder_y_mk=""
pvavifileparser_m_mk="/fileformats/avi/parser/build/make"
pvjitterbuffernode_opencore_y_mk=""
gsm_amr_headers_m_mk="/codecs_v2/audio/gsm_amr/common/dec/build/make"
pvaacparser_y_mk=""
pvaacffparsernode_m_mk="/nodes/pvaacffparsernode/build/make"
LIBS_extern_libs_shared="    "
LIBDIR_media_io_static="   "
getactualaacconfig_m_mk="/codecs_v2/audio/aac/dec/util/getactualaacconfig/build/make"
colorconvert_y_mk=""
pvgendatastruct_y_lib=""
LIBDIR_pvmi_shared="/pvmi/content_policy_manager/build/make /pvmi/content_policy_manager/plugins/oma1/passthru/build/make   /pvmi/content_policy_manager/plugins/common/build/make  /pvmi/media_io/pvmiofileoutput/build/make /pvmi/media_io/pvmi_mio_fileinput/build/make_pvauthor /pvmi/media_io/pvmi_mio_avi_wav_fileinput/build/make /pvmi/media_io/pvmio_comm_loopback/build/make /pvmi/recognizer/build/make /pvmi/recognizer/plugins/pvaacffrecognizer/build/make /pvmi/recognizer/plugins/pvamrffrecognizer/build/make   /pvmi/recognizer/plugins/pvmp3ffrecognizer/build/make /pvmi/recognizer/plugins/pvmp4ffrecognizer/build/make /pvmi/recognizer/plugins/pvwavffrecognizer/build/make    /pvmi/pvmf/build/make  "
pvoma1ffrecognizer_m_mk=""
csprng_y_mk=""
pvdivxffparsernode_y_lib=""
pvauthorengine_y_lib=""
pvrtspinterface_m_lib="-lpvrtspinterface"
pvmp3ff_y_mk=""
pv_aac_dec_imp_m_lib=""
rfc_3016_mk="rfc_3016.mk"
pvwmdrm_so_name=""
pvavcencnode_m_mk=""
pvasflocalpbinterface_m_mk=""
pvasxparser_y_lib=""
pvdivxffparsernode_m_mk=""
pvmp4decoder_m_mk="/codecs_v2/video/m4v_h263/dec/build/make"
rdt_parser_y_lib=""
pvvideodecnode_m_mk=""
pvmimeutils_y_mk=""
omx_m4venc_component_imp_m_mk=""
pvwavffrecognizer_m_lib="-lpvwavffrecognizer"
omx_amrenc_sharedlibrary_so_name="omx_amrenc_sharedlibrary"
pvrtsp_cli_eng_node_y_lib=""
gsmamrdecnode_m_lib=""
pvencoder_gsmamr_imp_m_lib=""
pvmp4ff_y_lib=""
pvamrffparsernode_y_mk=""
SOLIBDIRS_omx_wmadec_sharedlibrary=" "
wmadecoder_y_mk=""
SOLIBS_pvjanus=" "
pvmf_m_lib="-lpvmf"
LIBDIR_protocols_static="             "
ps_support_mk="pe_ps.mk"
pvavcdecnode_m_mk=""
pvra8decnode_m_lib=""
LIBDIR_codecs_utilities_shared="/codecs_v2/utilities/m4v_config_parser/build/make /codecs_v2/utilities/pv_config_parser/build/make /codecs_v2/utilities/colorconvert/build/make"
LIBS_omxenc_static="  "
broadcastpvr_support_mk=""
pvstreamingmanagernode_y_mk=""
pvmp4ffopencore_y_lib=""
protocolenginenode_m_lib=""
SOLIBS_opencore_mp4localreg="-lpvmp4reginterface -lpvmp4ffrecognizer"
omx_m4venc_component_y_lib=""
pvomxvideodecnode_m_lib="-lpvomxvideodecnode"
omx_wma_component_m_lib=""
pvpvrnode_m_lib=""
pv_avc_common_lib_m_lib="-lpv_avc_common_lib"
pvpvrff_m_lib=""
pvframemetadatautility_y_lib=""
pvmio_comm_loopback_m_mk="/pvmi/media_io/pvmio_comm_loopback/build/make"
LIBS_baselibs_shared="   -lpvgendatastruct -lpvmediadatastruct -lpvmimeutils -lthreadsafe_callback_ao -lpvthreadmessaging"
omx_avcenc_component_imp_m_mk=""
omx_amr_component_m_mk="/codecs_v2/omx/omx_amr/build/make_multithreaded"
pvwmdrmplat_so_name=""
pv_omx_interface_m_mk="/codecs_v2/omx/omx_sharedlibrary/interface/build/make"
pvdummyinputnode_m_mk="/nodes/pvdummyinputnode/build/make"
pvasxparser_m_mk=""
omx_avc_component_imp_m_lib=""
pvavch264enc_y_lib=""
pvmp3ff_m_lib="-lpvmp3ff"
oscl_m_mk="/oscl"
pvrvdecnode_y_lib=""
pvclientserversocketnode_m_mk="/nodes/pvclientserversocketnode/build/make"
PROTOCOL_PLUGINS="pe_dl_common.mk pe_ps.mk pe_pdl.mk  "
LIBDIR_nodes_shared="/nodes/pvfileoutputnode/build/make /nodes/pvmediaoutputnode/build/make /nodes/pvsocketnode/build/make  /nodes/pvprotocolenginenode/build/make_segments   /nodes/pvwavffparsernode/build/make   /nodes/pvomxencnode/build/make /nodes/pvomxbasedecnode/build/make /nodes/pvomxaudiodecnode/build/make /nodes/pvomxvideodecnode/build/make  /nodes/pvaacffparsernode/build/make  /nodes/pvamrffparsernode/build/make   /nodes/pvmp3ffparsernode/build/make  /nodes/pvmp4ffparsernode/build_opencore/make     /nodes/common/build/make  /nodes/pvmediainputnode/build/make_pvauthor  /nodes/pvmp4ffcomposernode/build_opencore/make     /nodes/pvdownloadmanagernode/build/make   /nodes/streaming/streamingmanager/build/make_segments   /modules/linux_rtsp/core/build/make /modules/linux_rtsp/node_registry/build/make /nodes/streaming/medialayernode/build/make  /nodes/streaming/jitterbuffernode/build/make  /nodes/pvcommsionode/build/make /nodes/pvclientserversocketnode/build/make /nodes/pvloopbacknode/build/make /nodes/pvvideoparsernode/build/make /nodes/pvdummyinputnode/build/make /nodes/pvdummyoutputnode/build/make "
pvavcdecoder_imp_m_mk=""
pvwmdrmdevinterface_m_lib=""
pvdownloadinterface_m_lib="-lpvdownloadinterface"
pvasfff_y_mk=""
pvstreamingmanagernode_segments_y_lib=""
pv_divxfile_parser_y_mk=""
omx_amr_component_imp_m_mk=""
csprng_y_lib=""
getactualaacconfig_for_static_m_lib=""
SOLIBS_omx_aacdec_sharedlibrary="-lomx_aac_component_lib -lpv_aac_dec"
omx_queue_m_lib="-lomx_queue_lib"
nodes_common_headers_y_mk=""
pvaacparser_y_lib=""
SOLIBDIRS_omx_sharedlibrary="/codecs_v2/omx/omx_common/build/make_multithreaded /codecs_v2/omx/omx_queue/build/make /codecs_v2/omx/omx_proxy/build/make /codecs_v2/omx/omx_baseclass/build/make /codecs_v2/omx/omx_sharedlibrary/interface/build/make"
omx_avcenc_component_m_mk="/codecs_v2/omx/omx_h264enc/build/make_multithreaded"
wmvdecoder_y_lib=""
pvamrffparsernode_y_lib=""
cml2_support_flag="USE_CML2_CONFIG"
MODS_opencore_rtsp="-lopencore_net_support -lopencore_player -lopencore_common"
pvclientserversocketnode_m_lib="-lpvclientserversocketnode"
pvsqlite_m_mk=""
REGISTER_OMX_AMR_COMPONENT=1
DYNAMIC_LOAD_OMX_MP3_COMPONENT=1
omx_aac_component_y_lib=""
pvavcdecoder_y_mk=""
getactualaacconfig_y_lib=""
pvrtsp_cli_eng_node_opencore_y_mk=""
SOLIBDIRS_pvwmdrm="   "
pvoma1lockstream_y_mk=""
omx_amrenc_component_y_mk=""
LIBS_omxjoint_static="   "
pvauthorengine_m_lib="-lpvauthorengine"
standard_bcast_ps_mk=""
pvmediaoutputnode_y_lib=""
pv2wayengine_y_mk=""
pvmio_comm_loopback_y_mk=""
pvmediaoutputnode_m_lib="-lpvmediaoutputnode"
SOLIBS_pvasfstreamingreg="n"
pvomxaudiodecnode_m_lib="-lpvomxaudiodecnode"
pvid3parcom_y_lib=""
pvwavffparsernode_y_lib=""
pvrtsp_cli_eng_node_opencore_y_lib=""
pvra8decoder_m_lib=""
pvcommsionode_m_lib="-lpvcommsionode"
DYNAMIC_LOAD_OMX_M4VENC_COMPONENT=1
pv324m_y_mk=""
LIBDIR_omxencimp_static=""
pvrmffparser_y_mk=""
pvmp4ffparsernode_y_mk=""
pvmp4ffrecognizer_y_mk=""
pvstreamingmanagernode_3gpp_y_mk=""
pvlatmpayloadparser_y_mk=""
protocolenginenode_segments_y_lib=""
MODS_omx_m4venc_sharedlibrary="-lomx_sharedlibrary -lopencore_common "
LIBDIR_omxdec_static="      "
pvthreadmessaging_m_lib="-lpvthreadmessaging"
pvmp4ff_y_mk=""
pvpvr_m_mk=""
MODS_omx_wmvdec_sharedlibrary="-lomx_sharedlibrary -lopencore_common"
omx_m4v_component_m_mk="/codecs_v2/omx/omx_m4v/build/make_multithreaded"
pvmediaoutputnode_m_mk="/nodes/pvmediaoutputnode/build/make"
pvvideoparsernode_m_mk="/nodes/pvvideoparsernode/build/make"
LIBDIR_omxencimp_shared=""
pvwmdrmplatinterface_m_mk=""
config_asf_mk=""
pvvideodecnode_y_lib=""
pvmp4ffparsernodeopencore_y_lib=""
SOLIBS_omx_m4venc_sharedlibrary="-lomx_m4venc_component_lib -lpvm4vencoder"
passthru_oma1_m_mk="/pvmi/content_policy_manager/plugins/oma1/passthru/build/make"
packetsources_default_plugins=" "
mp3decnode_y_mk=""
omx_queue_y_lib=""
pvmp3_imp_m_mk=""
cpm_headers_y_mk=""
pvdownloadinterface_m_mk="/modules/linux_download/core/build/make"
pvwavffrecognizer_m_mk="/pvmi/recognizer/plugins/pvwavffrecognizer/build/make"
pv_aac_dec_imp_m_mk=""
pvthreadmessaging_y_mk=""
pvasfff_y_lib=""
pvdownloadreginterface_m_lib="-lpvdownloadreginterface"
pvjanus_so_name=""
pvpvr_m_lib=""
omx_m4v_component_imp_m_mk=""
rtspunicast_support_mk="rtspunicast.mk"
mp3decnode_y_lib=""
pvmfrecognizer_m_mk="/pvmi/recognizer/build/make"
LIBDIR_media_io_shared="/pvmi/media_io/pvmiofileoutput/build/make /pvmi/media_io/pvmi_mio_fileinput/build/make_pvauthor /pvmi/media_io/pvmi_mio_avi_wav_fileinput/build/make /pvmi/media_io/pvmio_comm_loopback/build/make"
unicastpvr_support_mk=""
pvamrffparsernode_m_mk="/nodes/pvamrffparsernode/build/make"
pvmp3ffrecognizer_m_lib="-lpvmp3ffrecognizer"
LIBS_omxdecimp_shared=""
pvasfffparsernode_y_mk=""
http_support_mk=""
REGISTER_OMX_AVCENC_COMPONENT=1
pv_avc_common_lib_m_mk="/codecs_v2/video/avc_h264/common/build/make"
pvdummyoutputnode_y_mk=""
MODS_opencore_download="-lopencore_net_support -lopencore_player -lopencore_common"
pvasxparser_y_mk=""
pvstreamingmanagernode_segments_m_lib="-lpvstreamingmanagernode"
pvdecoder_gsmamr_imp_m_mk=""
pvomxbasedecnode_y_lib=""
pvmiofileinput_m_mk="/pvmi/media_io/pvmi_mio_fileinput/build/make_pvauthor"
pvwavffparsernode_m_mk="/nodes/pvwavffparsernode/build/make"
pvmedialayernode_opencore_y_lib=""
LIBS_audio_static="        "
colorconvert_m_mk="/codecs_v2/utilities/colorconvert/build/make"
LIBDIR_omxdec_shared="/codecs_v2/omx/omx_h264/build/make_multithreaded /codecs_v2/omx/omx_m4v/build/make_multithreaded  /codecs_v2/omx/omx_aac/build/make_multithreaded /codecs_v2/omx/omx_amr/build/make_multithreaded /codecs_v2/omx/omx_mp3/build/make_multithreaded "
opencore_rtsp_so_name="opencore_rtsp"
pvwmadecnode_m_mk=""
pvmp4ffcomposer_y_mk=""
pvmedialayernode_m_lib="-lpvmedialayernode"
pvomxencnode_m_lib="-lpvomxencnode"
pvclientserversocketnode_y_mk=""
pvdownloadmanagernode_y_lib=""
pvmp3ffparsernode_m_mk="/nodes/pvmp3ffparsernode/build/make"
pvdivxffrecognizer_y_lib=""
LIBDIR_codecs_utilities_static="  "
omx_amr_component_m_lib="-lomx_amr_component_lib"
pvstreamingmanagernode_y_lib=""
pvsdpparser_m_mk="/protocols/sdp/parser/build/make"
unit_test_m_lib=""
pvmedialayernode_opencore_y_mk=""
pvsqlite_y_mk=""
omx_queue_y_mk=""
pvauthorengine_m_mk="/engines/author/build/make"
pvgeneraltools_y_lib=""
pvmp4interface_m_lib="-lpvmp4interface"
MODS_pvasfstreaming="-lopencore_net_support -lopencore_player -lopencore_common -lpvasfcommon"
pvomxvideodecnode_y_mk=""
pv_avc_common_imp_lib_m_lib=""
pvdivxffrecognizer_y_mk=""
pvdbmanager_m_mk=""
pvrtsp_cli_eng_node_3gpp_y_lib=""
pvmediadatastruct_y_mk=""
pvstreamingmanagernode_segments_y_mk=""
MODS_pv=""
pvdownloadmanagernode_y_mk=""
pvplayer_engine_y_lib=""
USE_LOADABLE_MODULES=y
pvrmffrecognizer_y_mk=""
SOLIBS_pvwmdrm="   "
omx_sharedlibrary_so_name="omx_sharedlibrary"
SOLIBDIRS_opencore_rtsp="/modules/linux_rtsp/core/build/make /nodes/streaming/streamingmanager/build/make_segments /protocols/rtsp_parcom/build/make /protocols/rtsp_client_engine/build_opencore/make /protocols/rtp_payload_parser/build/make /protocols/rtp/build/make /nodes/streaming/jitterbuffernode/build/make /nodes/streaming/medialayernode/build/make /protocols/sdp/parser/build/make /protocols/sdp/common/build/make"
omx_avc_component_imp_m_mk=""
omx_avcenc_component_y_lib=""
rtprtcp_y_mk=""
MODS_pvasfcommon="-lopencore_player -lopencore_common"
pvrtsp_cli_eng_node_m_lib=""
SOLIBDIRS_omx_wmvdec_sharedlibrary=" "
pvpvxparser_m_mk=""
pvvideoparsernode_m_lib="-lpvvideoparsernode"
LIBDIR_module="/modules"
pventropysrc_y_mk=""
pvrmffparser_m_mk=""
pv_omx_interface_m_lib="-lpv_omx_interface"
SOLIBS_pvwmdrmplat="n"
pvmp4ffparsernode_y_lib=""
pvrmff_y_mk=""
pvmp4ffcomposernode_y_mk=""
pvm4vencoder_y_lib=""
pvomx_proxy_y_mk=""
pvgeneraltools_y_mk=""
pvrmffrecognizer_m_lib=""
pvsocketnode_y_mk=""
pvmediainputnode_m_mk="/nodes/pvmediainputnode/build/make_pvauthor"
pvrtsp_cli_eng_node_opencore_m_lib="-lpvrtsp_cli_eng_node"
mp4recognizer_utility_m_mk="/fileformats/mp4/parser/utils/mp4recognizer/build/make"
SOLIBDIRS_pv="/oscl   /codecs_v2/audio/aac/dec/build/make /codecs_v2/audio/mp3/dec/build/make  /codecs_v2/audio/gsm_amr/amr_nb/dec/build/make /codecs_v2/audio/gsm_amr/amr_nb/enc/build/make /codecs_v2/audio/gsm_amr/amr_nb/common/build/make /codecs_v2/audio/gsm_amr/amr_wb/dec/build/make /codecs_v2/video/avc_h264/dec/build/make /codecs_v2/video/avc_h264/common/build/make /codecs_v2/video/avc_h264/enc/build/make  /codecs_v2/video/m4v_h263/dec/build/make /codecs_v2/video/m4v_h263/enc/build/make  /codecs_v2/audio/aac/dec/util/getactualaacconfig/build/make /codecs_v2/utilities/colorconvert/build/make /codecs_v2/utilities/m4v_config_parser/build/make /codecs_v2/utilities/pv_config_parser/build/make /codecs_v2/omx/omx_h264/build/make_multithreaded /codecs_v2/omx/omx_m4v/build/make_multithreaded  /codecs_v2/omx/omx_aac/build/make_multithreaded /codecs_v2/omx/omx_amr/build/make_multithreaded /codecs_v2/omx/omx_mp3/build/make_multithreaded  /codecs_v2/omx/omx_common/build/make_multithreaded /codecs_v2/omx/omx_queue/build/make /codecs_v2/omx/omx_proxy/build/make /codecs_v2/omx/omx_baseclass/build/make /codecs_v2/omx/omx_mastercore/build/make_multithreaded /codecs_v2/omx/omx_sharedlibrary/interface/build/make /baselibs/threadsafe_callback_ao/build/make /baselibs/media_data_structures/build/make /baselibs/pv_mime_utils/build/make /baselibs/gen_data_structures/build/make /pvmi/pvmf/build/make /pvmi/content_policy_manager/build/make /pvmi/content_policy_manager/plugins/oma1/passthru/build/make /pvmi/media_io/pvmiofileoutput/build/make /fileformats/common/parser/build/make /fileformats/id3parcom/build/make  /fileformats/mp4/parser/build_opencore/make /fileformats/mp4/parser/utils/mp4recognizer/build/make  /fileformats/mp4/composer/build_opencore/make  /nodes/pvmp4ffcomposernode/build_opencore/make      /nodes/pvmediainputnode/build/make_pvauthor /nodes/pvmediaoutputnode/build/make /nodes/pvfileoutputnode/build/make /fileformats/rawgsmamr/parser/build/make /nodes/pvamrffparsernode/build/make  /pvmi/recognizer/plugins/pvamrffrecognizer/build/make /fileformats/rawaac/parser/build/make /nodes/pvaacffparsernode/build/make  /pvmi/recognizer/plugins/pvaacffrecognizer/build/make /fileformats/mp3/parser/build/make  /nodes/pvmp3ffparsernode/build/make /pvmi/recognizer/plugins/pvmp3ffrecognizer/build/make    /nodes/pvomxvideodecnode/build/make /nodes/pvomxaudiodecnode/build/make /nodes/pvomxbasedecnode/build/make        /nodes/pvwavffparsernode/build/make /pvmi/recognizer/plugins/pvwavffrecognizer/build/make     /pvmi/recognizer/build/make /pvmi/media_io/pvmi_mio_fileinput/build/make_pvauthor /pvmi/media_io/pvmi_mio_avi_wav_fileinput/build/make /engines/adapters/player/framemetadatautility/build/make /engines/player/build/make /engines/author/build/make /protocols/systems/3g-324m_pvterminal/build/make/ /engines/2way/build/make /fileformats/avi/parser/build/make /baselibs/thread_messaging/build/make /protocols/rtp_payload_parser/util/build/latmparser/make "
USE_DYNAMIC_LOAD_OMX_COMPONENTS=1
LIBDIR_omxenc_static="  "
pv_config_parser_m_mk="/codecs_v2/utilities/pv_config_parser/build/make"
pvamrffrecognizer_m_mk="/pvmi/recognizer/plugins/pvamrffrecognizer/build/make"
pv_avc_common_lib_y_lib=""
SOLIBDIRS_opencore_downloadreg="/modules/linux_download/node_registry/build/make"
SOLIBDIRS_pvasfstreamingreg="n"
colorconvert_m_lib="-lcolorconvert"
pvomxaudiodecnode_y_lib=""
pvamrwbdecoder_y_lib=""
pvjanusplugin_y_lib=""
rtppayloadparser_y_mk=""
pvid3parcom_m_mk="/fileformats/id3parcom/build/make"
wmvdecoder_imp_m_lib="n"
pvmp4ffrecognizer_y_lib=""
pvmtp_engine_y_mk=""
opencore_author_so_name="opencore_author"
omx_wma_component_y_mk=""
LIBS_protocols_shared="-lpv_http_parcom -lpvlatmpayloadparser  -lpvsdpparser  -lpv_rtsp_parcom   -lpvrtsp_cli_eng_node -lrtppayloadparser -lrtprtcp -lpv324m -lpvgeneraltools"
LIBDIR_video_shared="/codecs_v2/video/avc_h264/common/build/make /codecs_v2/video/avc_h264/dec/build/make  /codecs_v2/video/m4v_h263/dec/build/make  /codecs_v2/video/m4v_h263/enc/build/make /codecs_v2/video/avc_h264/enc/build/make"
SOLIBDIRS_opencore_rtspreg="/modules/linux_rtsp/node_registry/build/make"
omx_baseclass_m_lib="-lomx_baseclass_lib"
config_3gpp_asf_mk=""
pvasflocalpbreg_so_name=""
pvdummyinputnode_m_lib="-lpvdummyinputnode"
pvamrencnode_y_mk=""
pvwmadecnode_y_lib=""
pvoma1ffrecognizer_y_mk=""
unit_test_y_mk="/oscl/unit_test/build/make"
pvgendatastruct_m_mk="/baselibs/gen_data_structures/build/make"
pvmiofileinput_m_lib="-lpvmiofileinput"
realaudio_deinterleaver_y_mk=""
pv_aac_dec_m_lib="-lpv_aac_dec"
pvpvr_y_lib=""
omx_queue_m_mk="/codecs_v2/omx/omx_queue/build/make"
omx_avcenc_sharedlibrary_so_name="omx_avcenc_sharedlibrary"
pvdummyinputnode_y_lib=""
omx_m4v_component_y_mk=""
DYNAMIC_LOAD_OMX_AVC_COMPONENT=1
pvwav_y_lib=""
pvgendatastruct_y_mk=""
pvra8decnode_y_lib=""
omx_common_y_lib=""
LIBS_extern_libs_static="  "
pvmiofileoutput_y_lib=""
pvjitterbuffernode_plugins="jb_default.mk"
omx_wma_component_imp_m_mk="n"
pvmfrecognizer_y_mk=""
protocolenginenode_plugins="pe_dl_common.mk pe_ps.mk pe_pdl.mk  "
pvpvrnode_m_mk=""
gsm_amr_headers_y_mk=""
gsmamrdecnode_m_mk=""
protocolenginenode_y_lib=""
omx_m4venc_sharedlibrary_so_name="omx_m4venc_sharedlibrary"
pvmedialayernode_plugins_opencore_rtsp="ml_rtsp.mk"
pvaacffparsernode_m_lib="-lpvaacffparsernode"
REGISTER_OMX_M4V_COMPONENT=1
pvasfstreamingreginterface_m_lib=""
MODS_opencore_net_support="-lopencore_common"
cpm_y_mk=""
pvmp3_imp_m_lib=""
LIBDIR_cpm_shared="/pvmi/content_policy_manager/build/make /pvmi/content_policy_manager/plugins/oma1/passthru/build/make   /pvmi/content_policy_manager/plugins/common/build/make "
threadsafe_callback_ao_y_mk=""
MODS_pvjanus="-lopencore_player -lopencore_common -lpvwmdrm"
pvstreamingmanagernode_3gpp_y_lib=""
pvdummyoutputnode_y_lib=""
pvm4vencoder_y_mk=""
pvomxencnode_y_mk=""
asfrecognizer_utility_y_lib=""
omx_common_m_lib="-lomx_common_lib"
pv_aac_dec_m_mk="/codecs_v2/audio/aac/dec/build/make"
LIBS_oscl_shared="-losclbase -losclerror -losclmemory -losclproc -losclregcli -losclregserv -losclutil -losclio -lunit_test  -loscllib"
MODS_pvasflocalpb="-lopencore_player -lopencore_common -lpvasfcommon"
pvframemetadatautility_m_lib="-lpvframemetadatautility"
pvamrwbdecoder_m_lib="-lpvamrwbdecoder"
omx_mastercore_y_lib=""
pvmfrecognizer_y_lib=""
pvmp3ff_m_mk="/fileformats/mp3/parser/build/make"
rtppayloadparser_m_mk="/protocols/rtp_payload_parser/build/make"
LIBS_engines_shared="-lpvplayer_engine -lpvauthorengine -lpv2wayengine -lpvframemetadatautility"
SOLIBDIRS_pvasfcommon=" "
pvomx_proxy_y_lib=""
pvsdpparser_m_lib=""
omx_wmv_component_imp_m_mk="n"
pvasfstreaminginterface_m_lib=""
oscl_y_lib=""
pvwmvdecnode_m_lib=""
rtprtcp_m_mk="/protocols/rtp/build/make"
pv_http_parcom_m_mk="/protocols/http_parcom/build/make"
LIBDIR_omxenc_shared="/codecs_v2/omx/omx_amrenc/build/make_multithreaded /codecs_v2/omx/omx_m4venc/build/make_multithreaded /codecs_v2/omx/omx_h264enc/build/make_multithreaded"
LIBS_protocols_static="            "
pvrtppacketsourcenode_m_mk=""
omx_common_y_mk=""
opencore_net_support_so_name="opencore_net_support"
MODS_omx_avcdec_sharedlibrary="-lomx_sharedlibrary -lopencore_common"
aacdecnode_y_lib=""
LIBS_nodes_static="                                                 "
pvcommsionode_y_lib=""
pvasfffrecognizer_m_mk=""
pvlatmpayloadparser_y_lib=""
MODS_omx_amrdec_sharedlibrary="-lomx_sharedlibrary -lopencore_common"
pvmp4ffparsernode_m_mk=""
LIBS_video_static="      "
rtppayloadparser_plugins_pvasfstreaming="asf_payload_parser.mk"
SOLIBS_omx_mp3dec_sharedlibrary="-lomx_mp3_component_lib -lpvmp3"
pvpvxparser_y_mk=""
pvsdpparser_opencore_m_lib="-lpvsdpparser"
realmedia_payload_mk=""
pv_divxfile_parser_y_lib=""
pvfileparserutils_m_mk="/fileformats/common/parser/build/make"
omx_amr_component_y_mk=""
pv_amr_nb_common_imp_lib_m_lib=""
pvasfffparsernode_m_lib=""
omx_avc_component_y_mk=""
pvdbmanager_y_lib=""
pvasxparser_m_lib=""
rfc_3640_mk="rfc_3640.mk"
pvavifileparser_y_lib=""
pvrtsp_cli_eng_node_3gpp_y_mk=""
pv324m_y_lib=""
pvasflocalpbreginterface_m_lib=""
MODS_omx_sharedlibrary="-lopencore_common"
pvvideodecnode_y_mk=""
pvsocketnode_y_lib=""
pvamrffparsernode_m_lib="-lpvamrffparsernode"
pvomxencnode_m_mk="/nodes/pvomxencnode/build/make"
SOLIBDIRS_opencore_mp4local="/modules/linux_mp4/core/build/make  /nodes/pvmp4ffparsernode/build_opencore/make"
omx_m4venc_component_m_lib="-lomx_m4venc_component_lib"
pvmp4ffcomposernodeopencore_m_lib="-lpvmp4ffcomposernode"
rfc_3267_mk="rfc_3267.mk"
getactualaacconfig_y_mk=""
LIBS_media_io_shared="-lpvmiofileoutput -lpvmiofileinput -lpvmioaviwavfileinput -lpvmio_comm_loopback"
LIBS_nodes_shared="-lpvfileoutputnode -lpvmediaoutputnode -lpvsocketnode  -lprotocolenginenode   -lpvwavffparsernode   -lpvomxencnode -lpvomxbasedecnode -lpvomxaudiodecnode -lpvomxvideodecnode  -lpvaacffparsernode  -lpvamrffparsernode   -lpvmp3ffparsernode  -lpvmp4ffparsernode      -lpvmediainputnode  -lpvmp4ffcomposernode     -lpvdownloadmanagernode   -lpvstreamingmanagernode   -lpvrtspinterface -lpvrtspreginterface -lpvmedialayernode  -lpvjitterbuffernode  -lpvcommsionode -lpvclientserversocketnode -lpvloopbacknode -lpvvideoparsernode -lpvdummyinputnode -lpvdummyoutputnode "
LIBS_audio_shared="  -lpv_aac_dec -lpv_amr_nb_common_lib -lpvamrwbdecoder -lpvdecoder_gsmamr -lpvmp3  -lpvencoder_gsmamr"
pvloopbacknode_m_mk="/nodes/pvloopbacknode/build/make"
pvvideoparsernode_y_lib=""
pvavch264enc_m_mk="/codecs_v2/video/avc_h264/enc/build/make"
csprng_m_mk=""
pvmp3_y_lib=""
pvoma1lockstream_m_lib=""
MODS_pvasfstreamingreg="-lopencore_player -lopencore_common"
pvmf_y_lib=""
pvjitterbuffernode_m_mk="/nodes/streaming/jitterbuffernode/build/make"
pvmio_comm_loopback_y_lib=""
pvmp4decoder_imp_m_mk=""
pv_config_parser_y_lib=""
pvmp3ffrecognizer_y_lib=""
pvavcdecoder_y_lib=""
m4v_config_m_lib="-lm4v_config"
pv_rtsp_parcom_m_lib="-lpv_rtsp_parcom"
omx_aac_component_y_mk=""
protocolenginenode_plugins_pvdownload="pe_ps.mk pe_pdl.mk pe_dl_common.mk"
pvaacffrecognizer_m_lib="-lpvaacffrecognizer"
packetsources_default_y_mk=""
rdt_parser_m_lib=""
pvwmvdecnode_y_lib=""
LIBDIR_omxjointimp_static="n"
pvmtpdb_so_name=""
pvmioaviwavfileinput_y_lib=""
pvdecoder_gsmamr_y_lib=""
pvwmdrm_m_lib=""
cpm_m_mk="/pvmi/content_policy_manager/build/make"
omx_amrdec_sharedlibrary_so_name="omx_amrdec_sharedlibrary"
pvoma1lockstream_m_mk=""
pvomxbasedecnode_y_mk=""
pvloopbacknode_y_lib=""
realaudio_deinterleaver_m_lib=""
pv_amr_nb_common_lib_y_mk=""
REGISTER_OMX_AMRENC_COMPONENT=1
SOLIBS_pvasfcommon=" "
cpm_headers_m_mk="/pvmi/content_policy_manager/plugins/common/build/make"
opencore_mp4localreg_so_name="opencore_mp4localreg"
pvmp4ffrecognizer_m_mk="/pvmi/recognizer/plugins/pvmp4ffrecognizer/build/make"
LIBS_cpm_static="   "
SOLIBS_omx_m4vdec_sharedlibrary="-lomx_m4v_component_lib -lpvmp4decoder"
pvid3parcom_m_lib="-lpvid3parcom"
pvstreamingmanagernode_segments_m_mk="/nodes/streaming/streamingmanager/build/make_segments"
pvrtsp_cli_eng_node_y_mk=""
omx_avcenc_component_imp_m_lib=""
pvmp4ffcomposernodeopencore_y_mk=""
DYNAMIC_LOAD_OMX_H263ENC_COMPONENT=1
pvasflocalpb_so_name=""
pvsocketnode_m_lib="-lpvsocketnode"
SOLIBDIRS_opencore_common="/oscl  /codecs_v2/omx/omx_mastercore/build/make_multithreaded              /codecs_v2/audio/gsm_amr/common/dec/build/make /codecs_v2/video/avc_h264/common/build/make /codecs_v2/audio/gsm_amr/amr_nb/common/build/make   /fileformats/rawgsmamr/parser/build/make  /codecs_v2/audio/aac/dec/util/getactualaacconfig/build/make /codecs_v2/utilities/m4v_config_parser/build/make /codecs_v2/utilities/pv_config_parser/build/make /codecs_v2/utilities/colorconvert/build/make /baselibs/threadsafe_callback_ao/build/make /baselibs/media_data_structures/build/make /baselibs/pv_mime_utils/build/make /baselibs/gen_data_structures/build/make /pvmi/pvmf/build/make /nodes/pvfileoutputnode/build/make /nodes/pvmediainputnode/build/make_pvauthor /nodes/pvomxencnode/build/make /pvmi/media_io/pvmi_mio_fileinput/build/make_pvauthor /pvmi/media_io/pvmi_mio_avi_wav_fileinput/build/make /fileformats/avi/parser/build/make /baselibs/thread_messaging/build/make /pvmi/media_io/pvmiofileoutput/build/make /nodes/pvmediaoutputnode/build/make /nodes/pvomxvideodecnode/build/make /nodes/pvomxaudiodecnode/build/make /nodes/pvomxbasedecnode/build/make /protocols/rtp_payload_parser/util/build/latmparser/make /fileformats/wav/parser/build/make /fileformats/common/parser/build/make /nodes/common/build/make /engines/common/build/make /pvmi/content_policy_manager/plugins/common/build/make"
SOLIBS_opencore_common="-losclbase -losclerror -losclmemory -losclproc -losclregcli -losclregserv -losclutil -losclio -lunit_test  -loscllib -lomx_mastercore_lib              -lpv_avc_common_lib -lpv_amr_nb_common_lib   -lpvgsmamrparser  -lgetactualaacconfig -lm4v_config -lpv_config_parser -lcolorconvert -lthreadsafe_callback_ao -lpvmediadatastruct -lpvmimeutils -lpvgendatastruct -lpvmf -lpvfileoutputnode -lpvmediainputnode -lpvomxencnode -lpvmiofileinput -lpvmioaviwavfileinput -lpvavifileparser -lpvthreadmessaging -lpvmiofileoutput -lpvmediaoutputnode -lpvomxvideodecnode -lpvomxaudiodecnode -lpvomxbasedecnode -lpvlatmpayloadparser -lpvwav -lpvfileparserutils"
pvmp4decoder_y_lib=""
rtprtcp_m_lib="-lrtprtcp"
pvmimeutils_m_lib="-lpvmimeutils"
pvasfff_m_mk=""
pvgsmamrparser_m_mk="/fileformats/rawgsmamr/parser/build/make"
pvframemetadatautility_y_mk=""
pvmtp_engine_y_lib=""
omx_mp3_component_y_lib=""
rvdecoder_m_mk=""
pvmedialayernode_plugins="ml_default.mk"
pvjitterbuffernode_plugins_opencore_rtsp="jb_rtsp.mk"
omx_mastercore_y_mk=""
pvmp4ffcomposer_m_mk=""
pvasflocalpbreginterface_m_mk=""
pvasflocalpbinterface_m_lib=""
SOLIBS_omx_amrdec_sharedlibrary="-lomx_amr_component_lib -lpvdecoder_gsmamr -lpvamrwbdecoder"
pvra8decnode_y_mk=""
pv324m_common_headers_m_mk="/protocols/systems/common/build/make/"
pvwmdrmdevinterface_m_mk=""
REGISTER_OMX_MP3_COMPONENT=1
pvra8decoder_y_mk=""
rtppayloadparser_plugins="rfc_2429.mk rfc_3016.mk rfc_3267.mk rfc_3640.mk rfc_3984.mk  "
pvavcencnode_y_mk=""
optimized_bcast_ps_mk=""
pvmtp_engine_m_mk=""
pvstreamingmanagernode_3gpp_m_lib=""
pvplayer_engine_m_mk="/engines/player/build/make"
LIBDIR_engines_static="    "
LIBS_omxdec_shared="-lomx_avc_component_lib -lomx_m4v_component_lib  -lomx_aac_component_lib -lomx_amr_component_lib -lomx_mp3_component_lib "
LIBDIR_pvmi_static="                      "
pvsdpparser_y_mk=""
pv324m_common_headers_y_mk=""
pvthreadmessaging_y_lib=""
pvmp4decoder_y_mk=""
pvthreadmessaging_m_mk="/baselibs/thread_messaging/build/make"
pvmp4interface_m_mk="/modules/linux_mp4/core/build/make"
REGISTER_OMX_AVC_COMPONENT=1
pvaacffparsernode_y_lib=""
SOLIBDIRS_omx_amrenc_sharedlibrary="/codecs_v2/omx/omx_amrenc/build/make_multithreaded /codecs_v2/audio/gsm_amr/amr_nb/enc/build/make"
LIBS_tools_v2_shared="-lpvrtspinterface -lpvrtspreginterface   -lpvdownloadinterface -lpvdownloadreginterface -lpvmp4interface -lpvmp4reginterface  "
SOLIBS_pvasfstreaming=" -lpvstreamingmanagernode -lpvjitterbuffernode -lpvmedialayernode -lrtppayloadparser -lrtprtcp -lprotocolenginenode"
LIBS_pvmi_shared="-lcpm -lpassthru_oma1    -lpvmiofileoutput -lpvmiofileinput -lpvmioaviwavfileinput -lpvmio_comm_loopback -lpvmfrecognizer -lpvaacffrecognizer -lpvamrffrecognizer   -lpvmp3ffrecognizer -lpvmp4ffrecognizer -lpvwavffrecognizer    -lpvmf  "
pvmioaviwavfileinput_y_mk=""
omx_mp3_component_imp_m_mk=""
pvm4vencoder_imp_m_mk=""
pvwavffrecognizer_y_mk=""
m4v_config_y_lib=""
pvdownloadreginterface_m_mk="/modules/linux_download/node_registry/build/make"
pvrtppacketsourcenode_m_lib=""
DYNAMIC_LOAD_OMX_WMA_COMPONENT=0
omx_aac_component_imp_m_lib=""
pvmp3_y_mk=""
omx_wmv_component_y_mk=""
omx_mp3_component_imp_m_lib=""
SOLIBS_pvasflocalpb=" "
pvrtsp_cli_eng_node_3gpp_m_lib=""
pvwmvdecnode_y_mk=""
pvmp4ffparsernodeopencore_m_lib="-lpvmp4ffparsernode"
SOLIBDIRS_pvasflocalpb=" "
passthru_oma1_y_lib=""
pvamrencnode_m_mk=""
SOLIBS_opencore_download="-lprotocolenginenode -lpvdownloadmanagernode -lpvdownloadinterface"
oscllib_mk="/oscl/oscl/oscllib/build/make"
DYNAMIC_LOAD_OMX_H263_COMPONENT=1
pvomxencnode_y_lib=""
omx_wma_component_imp_m_lib="n"
getactualaacconfig_m_lib="-lgetactualaacconfig"
sdp_parser_mksegment_default=""
threadsafe_callback_ao_m_mk="/baselibs/threadsafe_callback_ao/build/make"
pvasfstreamingreginterface_m_mk=""
SOLIBDIRS_opencore_download="/nodes/pvprotocolenginenode/build/make_segments /nodes/pvdownloadmanagernode/build/make /modules/linux_download/core/build/make"
mp3decnode_m_lib=""
pvasf_streamingreg_so_name=""
pvcommsionode_y_mk=""
pvpvxparser_y_lib=""
omx_baseclass_m_mk="/codecs_v2/omx/omx_baseclass/build/make"
LIBDIR_oscl_shared="/oscl "
pvencoder_gsmamr_imp_m_mk=""
pvra8decoder_y_lib=""
nodes_common_headers_m_mk="/nodes/common/build/make"
LIBDIR_recognizer_shared="/pvmi/recognizer/build/make /pvmi/recognizer/plugins/pvaacffrecognizer/build/make /pvmi/recognizer/plugins/pvamrffrecognizer/build/make   /pvmi/recognizer/plugins/pvmp3ffrecognizer/build/make /pvmi/recognizer/plugins/pvmp4ffrecognizer/build/make /pvmi/recognizer/plugins/pvwavffrecognizer/build/make  "
MODS_opencore_author="-lopencore_common"
omx_wmv_component_m_lib=""
MODS_pvasflocalpbreg="-lopencore_common -lpvasfcommon"
pvrmffparsernode_m_lib=""
pvrtsp_cli_eng_node_m_mk=""
packetsources_default_m_mk=""
pvaacffrecognizer_m_mk="/pvmi/recognizer/plugins/pvaacffrecognizer/build/make"
pvoma1lockstream_y_lib=""
LIBS_baselibs_static="       "
pv_divxfile_parser_m_mk=""
LIBDIR_protocols_shared="/protocols/http_parcom/build/make /protocols/rtp_payload_parser/util/build/latmparser/make /protocols/sdp/parser/build/make /protocols/sdp/common/build/make  /protocols/rtsp_parcom/build/make   /protocols/rtsp_client_engine/build_opencore/make /protocols/rtp_payload_parser/build/make /protocols/rtp/build/make /protocols/systems/3g-324m_pvterminal/build/make/ /protocols/systems/common/build/make/ /protocols/systems/tools/general/build/make"
LIBS_media_io_static="   "
pv_avc_common_imp_lib_m_mk=""
SOLIBDIRS_opencore_2way="/engines/2way/build/make /protocols/systems/3g-324m_pvterminal/build/make/ /nodes/pvvideoparsernode/build/make /nodes/pvcommsionode/build/make /pvmi/media_io/pvmio_comm_loopback/build/make /protocols/systems/common/build/make/ /protocols/systems/tools/general/build/make "
pvmfrecognizer_m_lib="-lpvmfrecognizer"
pvencoder_gsmamr_y_mk=""
pvmedialayernode_y_mk=""
pvoma1ffrecognizer_y_lib=""
pvrtsp_cli_eng_node_3gpp_m_mk=""
protocolenginenode_plugins_pvasfstreaming="pe_http.mk"
pvdbmanager_y_mk=""
pv_amr_nb_common_imp_lib_m_mk=""
passthru_oma1_m_lib="-lpassthru_oma1"
LIBDIR_extern_libs_static="  "
pvwmdrm_y_lib=""
opencore_common_so_name="opencore_common"
pvmp4ffcomposernodeopencore_m_mk="/nodes/pvmp4ffcomposernode/build_opencore/make"
pvrmff_y_lib=""
omx_mp3_component_y_mk=""
SOLIBS_opencore_2way="-lpv2wayengine -lpv324m -lpvvideoparsernode -lpvcommsionode -lpvmio_comm_loopback -lpvgeneraltools "
SOLIBS_opencore_player="        -lcpm -lpassthru_oma1  -lpvid3parcom -lpvamrffparsernode -lpvamrffrecognizer -lpvmp3ff  -lpvmp3ffparsernode -lpvmp3ffrecognizer  -lpvmp4ff -lmp4recognizer_utility -lpvaacparser -lpvaacffparsernode  -lpvaacffrecognizer -lpvwavffparsernode -lpvwavffrecognizer           -lpvmfrecognizer   -lpvframemetadatautility -lpvplayer_engine"
MODS_opencore_mp4localreg="-lopencore_player -lopencore_common"
rdt_parser_y_mk=""
LIBDIR_packetsources_static="n"
SOLIBDIRS_omx_amrdec_sharedlibrary="/codecs_v2/omx/omx_amr/build/make_multithreaded /codecs_v2/audio/gsm_amr/amr_nb/dec/build/make /codecs_v2/audio/gsm_amr/amr_wb/dec/build/make"
pvjanusplugininterface_m_mk=""
pvmedialayernode_plugins_pvasfstreaming="ml_asf.mk"
pv_rtsp_parcom_y_mk=""
pvomxvideodecnode_m_mk="/nodes/pvomxvideodecnode/build/make"
pvmiofileoutput_m_mk="/pvmi/media_io/pvmiofileoutput/build/make"
aacdecnode_m_lib=""
pvcrypto_m_mk=""
rtppayloadparser_plugins_pvrtsp="rfc_2429.mk rfc_3016.mk rfc_3267.mk rfc_3640.mk rfc_3984.mk"
DYNAMIC_LOAD_OMX_AVCENC_COMPONENT=1
pvvideoencnode_m_lib=""
USE_OMX_ENC_NODE=1
pvamrwbdecoder_y_mk=""
mp4recognizer_utility_y_lib=""
pvrmffparsernode_y_lib=""
DYNAMIC_LOAD_OMX_M4V_COMPONENT=1
pvmp4ffparsernodeopencore_y_mk=""
pvmp4ffopencore_y_mk=""
LIBS_video_shared="-lpv_avc_common_lib -lpvavcdecoder  -lpvmp4decoder  -lpvm4vencoder -lpvavch264enc"
colorconvert_y_lib=""
pvm4vencoder_m_lib="-lpvm4vencoder"
pv_aac_dec_y_lib=""
omx_mp3dec_sharedlibrary_so_name="omx_mp3dec_sharedlibrary"
omx_wma_component_m_mk=""
SOLIBS_pvasflocalpbreg=" "
opencore_2way_so_name="opencore_2way"
pv_http_parcom_y_mk=""
pvgsmamrparser_y_lib=""
pvfileoutputnode_m_lib="-lpvfileoutputnode"
pvmedialayernode_opencore_m_mk=""
omx_wmvdec_sharedlibrary_so_name=""
opencore_rtspreg_so_name="opencore_rtspreg"
rfc_2429_mk="rfc_2429.mk"
SOLIBDIRS_opencore_net_support="/nodes/pvsocketnode/build/make /protocols/http_parcom/build/make"
LIBDIR_audio_static="         "
pvoma1ffrecognizer_m_lib=""
wmvdecoder_m_mk=""
rtprtcp_y_lib=""
rfc_3984_mk="rfc_3984.mk"
LIBDIR_shared="/oscl     /baselibs/gen_data_structures/build/make /baselibs/media_data_structures/build/make /baselibs/pv_mime_utils/build/make /baselibs/threadsafe_callback_ao/build/make /baselibs/thread_messaging/build/make /codecs_v2/omx/omx_h264/build/make_multithreaded /codecs_v2/omx/omx_m4v/build/make_multithreaded  /codecs_v2/omx/omx_aac/build/make_multithreaded /codecs_v2/omx/omx_amr/build/make_multithreaded /codecs_v2/omx/omx_mp3/build/make_multithreaded  /codecs_v2/omx/omx_amrenc/build/make_multithreaded /codecs_v2/omx/omx_m4venc/build/make_multithreaded /codecs_v2/omx/omx_h264enc/build/make_multithreaded /codecs_v2/omx/omx_common/build/make_multithreaded /codecs_v2/omx/omx_queue/build/make /codecs_v2/omx/omx_proxy/build/make /codecs_v2/omx/omx_baseclass/build/make /codecs_v2/omx/omx_mastercore/build/make_multithreaded /codecs_v2/omx/omx_sharedlibrary/interface/build/make   /codecs_v2/audio/aac/dec/build/make /codecs_v2/audio/gsm_amr/amr_nb/common/build/make /codecs_v2/audio/gsm_amr/amr_wb/dec/build/make /codecs_v2/audio/gsm_amr/amr_nb/dec/build/make /codecs_v2/audio/mp3/dec/build/make  /codecs_v2/audio/gsm_amr/common/dec/build/make /codecs_v2/audio/gsm_amr/amr_nb/enc/build/make /codecs_v2/video/avc_h264/common/build/make /codecs_v2/video/avc_h264/dec/build/make  /codecs_v2/video/m4v_h263/dec/build/make  /codecs_v2/video/m4v_h263/enc/build/make /codecs_v2/video/avc_h264/enc/build/make /codecs_v2/utilities/m4v_config_parser/build/make /codecs_v2/utilities/pv_config_parser/build/make /codecs_v2/utilities/colorconvert/build/make /fileformats/common/parser/build/make /fileformats/id3parcom/build/make  /fileformats/wav/parser/build/make  /fileformats/avi/parser/build/make  /fileformats/mp3/parser/build/make /fileformats/rawaac/parser/build/make /fileformats/rawgsmamr/parser/build/make    /fileformats/mp4/parser/utils/mp4recognizer/build/make /fileformats/mp4/parser/build_opencore/make  /fileformats/mp4/composer/build_opencore/make    /protocols/http_parcom/build/make /protocols/rtp_payload_parser/util/build/latmparser/make /protocols/sdp/parser/build/make /protocols/sdp/common/build/make  /protocols/rtsp_parcom/build/make   /protocols/rtsp_client_engine/build_opencore/make /protocols/rtp_payload_parser/build/make /protocols/rtp/build/make /protocols/systems/3g-324m_pvterminal/build/make/ /protocols/systems/common/build/make/ /protocols/systems/tools/general/build/make /pvmi/content_policy_manager/build/make /pvmi/content_policy_manager/plugins/oma1/passthru/build/make   /pvmi/content_policy_manager/plugins/common/build/make  /pvmi/media_io/pvmiofileoutput/build/make /pvmi/media_io/pvmi_mio_fileinput/build/make_pvauthor /pvmi/media_io/pvmi_mio_avi_wav_fileinput/build/make /pvmi/media_io/pvmio_comm_loopback/build/make /pvmi/recognizer/build/make /pvmi/recognizer/plugins/pvaacffrecognizer/build/make /pvmi/recognizer/plugins/pvamrffrecognizer/build/make   /pvmi/recognizer/plugins/pvmp3ffrecognizer/build/make /pvmi/recognizer/plugins/pvmp4ffrecognizer/build/make /pvmi/recognizer/plugins/pvwavffrecognizer/build/make    /pvmi/pvmf/build/make        /nodes/pvfileoutputnode/build/make /nodes/pvmediaoutputnode/build/make /nodes/pvsocketnode/build/make  /nodes/pvprotocolenginenode/build/make_segments   /nodes/pvwavffparsernode/build/make   /nodes/pvomxencnode/build/make /nodes/pvomxbasedecnode/build/make /nodes/pvomxaudiodecnode/build/make /nodes/pvomxvideodecnode/build/make  /nodes/pvaacffparsernode/build/make  /nodes/pvamrffparsernode/build/make   /nodes/pvmp3ffparsernode/build/make  /nodes/pvmp4ffparsernode/build_opencore/make     /nodes/common/build/make  /nodes/pvmediainputnode/build/make_pvauthor  /nodes/pvmp4ffcomposernode/build_opencore/make     /nodes/pvdownloadmanagernode/build/make   /nodes/streaming/streamingmanager/build/make_segments   /modules/linux_rtsp/core/build/make /modules/linux_rtsp/node_registry/build/make /nodes/streaming/medialayernode/build/make  /nodes/streaming/jitterbuffernode/build/make  /nodes/pvcommsionode/build/make /nodes/pvclientserversocketnode/build/make /nodes/pvloopbacknode/build/make /nodes/pvvideoparsernode/build/make /nodes/pvdummyinputnode/build/make /nodes/pvdummyoutputnode/build/make  /engines/player/build/make /engines/author/build/make /engines/2way/build/make /engines/common/build/make /engines/adapters/player/framemetadatautility/build/make /modules/linux_rtsp/core/build/make /modules/linux_rtsp/node_registry/build/make   /modules/linux_download/core/build/make /modules/linux_download/node_registry/build/make /modules/linux_mp4/core/build/make /modules/linux_mp4/node_registry/build/make  "
pdl_support_mk="pe_pdl.mk"
wmadecoder_m_mk=""
omx_aacdec_sharedlibrary_so_name="omx_aacdec_sharedlibrary"
pv_so_name=""
asf_payload_mk=""
pvmioaviwavfileinput_m_lib="-lpvmioaviwavfileinput"
protocolenginenode_segments_y_mk=""
pvamrencnode_m_lib=""
pvrvdecnode_y_mk=""
pvmp3ffrecognizer_m_mk="/pvmi/recognizer/plugins/pvmp3ffrecognizer/build/make"
LIBDIR_omxjoint_shared="/codecs_v2/omx/omx_common/build/make_multithreaded /codecs_v2/omx/omx_queue/build/make /codecs_v2/omx/omx_proxy/build/make /codecs_v2/omx/omx_baseclass/build/make /codecs_v2/omx/omx_mastercore/build/make_multithreaded /codecs_v2/omx/omx_sharedlibrary/interface/build/make"
pv324m_m_mk="/protocols/systems/3g-324m_pvterminal/build/make/"
pvavch264enc_imp_m_mk=""
LIBS_shared="-losclbase -losclerror -losclmemory -losclproc -losclregcli -losclregserv -losclutil -losclio -lunit_test  -loscllib    -lpvgendatastruct -lpvmediadatastruct -lpvmimeutils -lthreadsafe_callback_ao -lpvthreadmessaging -lomx_avc_component_lib -lomx_m4v_component_lib  -lomx_aac_component_lib -lomx_amr_component_lib -lomx_mp3_component_lib  -lomx_amrenc_component_lib -lomx_m4venc_component_lib -lomx_avcenc_component_lib -lomx_common_lib -lomx_queue_lib -lpvomx_proxy_lib -lomx_baseclass_lib -lomx_mastercore_lib -lpv_omx_interface   -lpv_aac_dec -lpv_amr_nb_common_lib -lpvamrwbdecoder -lpvdecoder_gsmamr -lpvmp3  -lpvencoder_gsmamr -lpv_avc_common_lib -lpvavcdecoder  -lpvmp4decoder  -lpvm4vencoder -lpvavch264enc -lm4v_config -lpv_config_parser -lcolorconvert -lpvfileparserutils -lpvid3parcom  -lpvwav  -lpvavifileparser  -lpvmp3ff -lpvaacparser -lpvgsmamrparser   -lmp4recognizer_utility -lpvmp4ff  -lpvmp4ffcomposer    -lpv_http_parcom -lpvlatmpayloadparser  -lpvsdpparser  -lpv_rtsp_parcom   -lpvrtsp_cli_eng_node -lrtppayloadparser -lrtprtcp -lpv324m -lpvgeneraltools -lcpm -lpassthru_oma1    -lpvmiofileoutput -lpvmiofileinput -lpvmioaviwavfileinput -lpvmio_comm_loopback -lpvmfrecognizer -lpvaacffrecognizer -lpvamrffrecognizer   -lpvmp3ffrecognizer -lpvmp4ffrecognizer -lpvwavffrecognizer    -lpvmf        -lpvfileoutputnode -lpvmediaoutputnode -lpvsocketnode  -lprotocolenginenode   -lpvwavffparsernode   -lpvomxencnode -lpvomxbasedecnode -lpvomxaudiodecnode -lpvomxvideodecnode  -lpvaacffparsernode  -lpvamrffparsernode   -lpvmp3ffparsernode  -lpvmp4ffparsernode      -lpvmediainputnode  -lpvmp4ffcomposernode     -lpvdownloadmanagernode   -lpvstreamingmanagernode   -lpvrtspinterface -lpvrtspreginterface -lpvmedialayernode  -lpvjitterbuffernode  -lpvcommsionode -lpvclientserversocketnode -lpvloopbacknode -lpvvideoparsernode -lpvdummyinputnode -lpvdummyoutputnode  -lpvplayer_engine -lpvauthorengine -lpv2wayengine -lpvframemetadatautility -lpvrtspinterface -lpvrtspreginterface   -lpvdownloadinterface -lpvdownloadreginterface -lpvmp4interface -lpvmp4reginterface  "
omx_amrenc_component_y_lib=""
pvgeneraltools_m_lib="-lpvgeneraltools"
REGISTER_OMX_AAC_COMPONENT=1
SOLIBDIRS_pvwmdrmplat="n"
pvwavffparsernode_m_lib="-lpvwavffparsernode"
LIBS_packetsources_shared="n"
pvstreamingmanagernode_plugins_opencore_rtsp="3gpp.mk rtspunicast.mk"
LIBS_omxjoint_shared="-lomx_common_lib -lomx_queue_lib -lpvomx_proxy_lib -lomx_baseclass_lib -lomx_mastercore_lib -lpv_omx_interface"
pvasfffrecognizer_m_lib=""
SOLIBS_pv="-losclbase -losclerror -losclmemory -losclproc -losclregcli -losclregserv -losclutil -losclio -lunit_test  -loscllib  -lpv_aac_dec -lpvmp3  -lpvdecoder_gsmamr -lpvencoder_gsmamr -lpv_amr_nb_common_lib -lpvamrwbdecoder -lpvavcdecoder -lpv_avc_common_lib -lpvavch264enc  -lpvmp4decoder -lpvm4vencoder  -lgetactualaacconfig -lcolorconvert -lm4v_config -lpv_config_parser -lomx_avc_component_lib -lomx_m4v_component_lib  -lomx_aac_component_lib -lomx_amr_component_lib -lomx_mp3_component_lib  -lomx_common_lib -lomx_queue_lib -lpvomx_proxy_lib -lomx_baseclass_lib -lomx_mastercore_lib -lpv_omx_interface -lthreadsafe_callback_ao -lpvmediadatastruct -lpvmimeutils -lpvgendatastruct -lpvmf -lcpm -lpassthru_oma1 -lpvmiofileoutput -lpvfileparserutils -lpvid3parcom  -lpvmp4ff -lmp4recognizer_utility  -lpvmp4ffcomposer  -lpvmp4ffcomposernode      -lpvmediainputnode -lpvmediaoutputnode -lpvfileoutputnode -lpvgsmamrparser -lpvamrffparsernode  -lpvamrffrecognizer -lpvaacparser -lpvaacffparsernode  -lpvaacffrecognizer -lpvmp3ff  -lpvmp3ffparsernode -lpvmp3ffrecognizer    -lpvomxvideodecnode -lpvomxaudiodecnode -lpvomxbasedecnode        -lpvwavffparsernode -lpvwavffrecognizer     -lpvmfrecognizer -lpvmiofileinput -lpvmioaviwavfileinput -lpvframemetadatautility -lpvplayer_engine -lpvauthorengine -lpv324m -lpv2wayengine -lpvavifileparser -lpvthreadmessaging -lpvlatmpayloadparser "
pvmp3ffparsernode_y_lib=""
pvjitterbuffernode_opencore_m_lib=""
MODS_opencore_player="-lopencore_common"
pvjitterbuffernode_m_lib="-lpvjitterbuffernode"
MODS_pvwmdrmplat="-lopencore_player -lopencore_common"
pvm4vencoder_imp_m_lib=""
cpm_y_lib=""
omx_wmadec_sharedlibrary_so_name=""
pvgsmamrparser_y_mk=""
rtppayloadparser_y_lib=""
pvra8decoder_m_mk=""
pvrmffparser_y_lib=""
pvmp4decoder_m_lib="-lpvmp4decoder"
omx_avc_component_m_lib="-lomx_avc_component_lib"
MODS_omx_avcenc_sharedlibrary="-lomx_sharedlibrary -lopencore_common "
pvwmvdecnode_m_mk=""
pvwav_y_mk=""
wmadecoder_imp_m_mk="n"
MODS_opencore_downloadreg="-lopencore_player -lopencore_common"
pvmiofileoutput_y_mk=""
packetsources_default_m_lib=""
pvaacparser_m_mk="/fileformats/rawaac/parser/build/make"
omx_aac_component_m_lib="-lomx_aac_component_lib"
SOLIBS_omx_wmadec_sharedlibrary=" "
SOLIBS_opencore_rtsp="-lpvrtspinterface -lpvstreamingmanagernode -lpv_rtsp_parcom -lpvrtsp_cli_eng_node -lrtppayloadparser -lrtprtcp -lpvjitterbuffernode -lpvmedialayernode  -lpvsdpparser"
omx_amrenc_component_imp_m_lib=""
pvjanusplugin_m_lib=""
config_3gpp_mk="3gpp.mk"
pvaacffrecognizer_y_mk=""
pvvideoencnode_y_lib=""
pvaacffparsernode_y_mk=""
pvomx_proxy_m_lib="-lpvomx_proxy_lib"
pvrvdecnode_m_mk=""
pvavcdecoder_imp_m_lib=""
pvframemetadatautility_m_mk="/engines/adapters/player/framemetadatautility/build/make"
LIBS_omxjointimp_static="n"
pvmediadatastruct_y_lib=""
MODS_opencore_mp4local="-lopencore_common -lopencore_player"
pvavch264enc_y_mk=""
rtppayloadparser_m_lib="-lrtppayloadparser"
pvcrypto_y_mk=""
pvmp4ffparsernodeopencore_m_mk="/nodes/pvmp4ffparsernode/build_opencore/make"
SOLIBDIRS_pvmtpdb="  "
LIBDIR_codecs_v2_static="                                 "
omx_m4v_component_imp_m_lib=""
pvmp4ffcomposeropencore_y_mk=""
oscl_y_mk=""
MODS_pvmtpdb="-lopencore_player -lopencore_common -lpvwmdrm -lpvasfcommon -lpvasflocalpbreg"
pvdivxffparsernode_y_mk=""
LIBS_codecs_utilities_shared="-lm4v_config -lpv_config_parser -lcolorconvert"
MODS_omx_m4vdec_sharedlibrary="-lomx_sharedlibrary -lopencore_common "
LIBS_omxdecimp_static=""
engines_common_headers_m_mk="/engines/common/build/make"
pvmp4ff_m_lib=""
pv2wayengine_m_mk="/engines/2way/build/make"
MODS_opencore_rtspreg="-lopencore_player -lopencore_common"
pvmtp_engine_m_lib=""
pvdivxffparsernode_m_lib=""
aacdecnode_y_mk=""
pvasfffrecognizer_y_mk=""
wmadecoder_y_lib=""
engines_common_headers_y_mk=""
MODS_omx_mp3dec_sharedlibrary="-lomx_sharedlibrary -lopencore_common"
asfrecognizer_utility_m_mk=""
oscllib_lib="-loscllib"
pvrmffparsernode_m_mk=""
realaudio_deinterleaver_m_mk=""
pvauthorengine_y_mk=""
LIBDIR_baselibs_static="       "
pvwav_m_lib="-lpvwav"
omx_avc_component_y_lib=""
pvmp4decoder_imp_m_lib=""
pv_rtsp_parcom_y_lib=""
pvmedialayernode_opencore_m_lib=""
pvmioaviwavfileinput_m_mk="/pvmi/media_io/pvmi_mio_avi_wav_fileinput/build/make"
omx_mp3_component_m_mk="/codecs_v2/omx/omx_mp3/build/make_multithreaded"
pvwmdrm_y_mk=""
LIBDIR_static=" /oscl/unit_test/build/make                                                                                                                                                              "
SOLIBS_opencore_mp4local="-lpvmp4interface  -lpvmp4ffparsernode"
config_3gpp_pvr_asf_mk=""
pvavch264enc_imp_m_lib=""
pvloopbacknode_m_lib="-lpvloopbacknode"
LIBS_omxdec_static="         "
pvwmadecnode_m_lib=""
REGISTER_OMX_WMA_COMPONENT=0
pvrmffrecognizer_m_mk=""
SOLIBS_opencore_net_support="-lpvsocketnode -lpv_http_parcom"
pv_http_parcom_m_lib="-lpv_http_parcom"
pvmediadatastruct_m_lib="-lpvmediadatastruct"
pvlatmpayloadparser_m_lib="-lpvlatmpayloadparser"
pv_config_parser_y_mk=""
pvstreamingmanagernode_plugins="  3gpp.mk rtspunicast.mk    "
TARGET_shared=" opencore_common opencore_author opencore_player opencore_2way omx_sharedlibrary omx_avcdec_sharedlibrary omx_m4vdec_sharedlibrary  omx_aacdec_sharedlibrary omx_amrdec_sharedlibrary omx_mp3dec_sharedlibrary  omx_avcenc_sharedlibrary omx_m4venc_sharedlibrary omx_amrenc_sharedlibrary opencore_net_support opencore_downloadreg opencore_download opencore_rtspreg opencore_rtsp      opencore_mp4localreg opencore_mp4local     "
pvsdpparser_opencore_y_lib=""
omx_aac_component_m_mk="/codecs_v2/omx/omx_aac/build/make_multithreaded"
LIBS_oscl_static=" -lunit_test"
pvomxbasedecnode_m_lib="-lpvomxbasedecnode"
pvfileoutputnode_m_mk="/nodes/pvfileoutputnode/build/make"
wmadecoder_imp_m_lib="n"
pv_rtsp_parcom_m_mk="/protocols/rtsp_parcom/build/make"
ftdl_support_mk=""
omx_wma_component_y_lib=""
pv_config_parser_m_lib="-lpv_config_parser"
m4v_config_y_mk=""
pvcrypto_m_lib=""
SOLIBDIRS_omx_avcdec_sharedlibrary="/codecs_v2/omx/omx_h264/build/make_multithreaded /codecs_v2/video/avc_h264/dec/build/make"
LIBS_fileformats_shared="-lpvfileparserutils -lpvid3parcom  -lpvwav  -lpvavifileparser  -lpvmp3ff -lpvaacparser -lpvgsmamrparser   -lmp4recognizer_utility -lpvmp4ff  -lpvmp4ffcomposer   "
gsmamrdecnode_y_mk=""
LIBS_engines_static="   "
pvjanusplugin_m_mk=""
LIBS_static=" -lunit_test                                                                                                                                                           "
pvsqlite_y_lib=""
pvasfffparsernode_y_lib=""
pvmp3_m_lib="-lpvmp3"
SOLIBDIRS_opencore_player="        /pvmi/content_policy_manager/build/make /pvmi/content_policy_manager/plugins/oma1/passthru/build/make  /fileformats/id3parcom/build/make /nodes/pvamrffparsernode/build/make /pvmi/recognizer/plugins/pvamrffrecognizer/build/make /fileformats/mp3/parser/build/make  /nodes/pvmp3ffparsernode/build/make /pvmi/recognizer/plugins/pvmp3ffrecognizer/build/make  /fileformats/mp4/parser/build_opencore/make /fileformats/mp4/parser/utils/mp4recognizer/build/make /fileformats/rawaac/parser/build/make /nodes/pvaacffparsernode/build/make  /pvmi/recognizer/plugins/pvaacffrecognizer/build/make /nodes/pvwavffparsernode/build/make /pvmi/recognizer/plugins/pvwavffrecognizer/build/make           /pvmi/recognizer/build/make   /engines/adapters/player/framemetadatautility/build/make /engines/player/build/make"
pvfileoutputnode_y_lib=""
pvstreamingmanagernode_plugins_pvasfstreaming="asf.mk mshttp.mk"
omx_avcdec_sharedlibrary_so_name="omx_avcdec_sharedlibrary"
pvsdpparser_plugins=" sdp_opencore.mk"
LIBS_recognizer_static="         "
pvencoder_gsmamr_m_lib="-lpvencoder_gsmamr"
SOLIBS_omx_amrenc_sharedlibrary="-lomx_amrenc_component_lib -lpvencoder_gsmamr"
pvplayer_engine_m_lib="-lpvplayer_engine"
pvpvxparser_m_lib=""
pvmp3ffparsernode_m_lib="-lpvmp3ffparsernode"
passthru_oma1_y_mk=""
pvmp4ffcomposernode_y_lib=""
gsmamrdecnode_y_lib=""
LIBDIR_cpm_static="    "
pvrtspinterface_m_mk="/modules/linux_rtsp/core/build/make"
wmvdecoder_m_lib=""
opencore_downloadreg_so_name="opencore_downloadreg"
omx_avcenc_component_y_mk=""
pvra8decnode_m_mk=""
pvamrwbdecoder_imp_m_lib=""
oscl_m_lib="-losclbase -losclerror -losclmemory -losclproc -losclregcli -losclregserv -losclutil -losclio -lunit_test"
CONFIG_FLAGS="USE_CML2_CONFIG"
pvomxbasedecnode_m_mk="/nodes/pvomxbasedecnode/build/make"
SOLIBDIRS_pvasflocalpbreg=" "
pvasfffrecognizer_y_lib=""
mp4recognizer_utility_y_mk=""
pvmediainputnode_y_mk=""
REGISTER_OMX_H263_COMPONENT=1
pvmp3_m_mk="/codecs_v2/audio/mp3/dec/build/make"
LIBS_codecs_utilities_static="  "
LIBDIR_omxjointimp_shared="/codecs_v2/omx/omx_mastercore/build/make_multithreaded"
DRMCONFIG=""
pvwavffparsernode_y_mk=""
SOLIBDIRS_omx_aacdec_sharedlibrary="/codecs_v2/omx/omx_aac/build/make_multithreaded /codecs_v2/audio/aac/dec/build/make"
pvmp4reginterface_m_lib="-lpvmp4reginterface"
pvavcdecnode_m_lib=""
pvpvrnode_y_lib=""
unit_test_y_lib="-lunit_test"
pvpvrff_y_lib=""
pvrfileplayback_support_mk=""
pvrvdecnode_m_lib=""
omx_aac_component_imp_m_mk=""
pvwav_m_mk="/fileformats/wav/parser/build/make"
omx_m4venc_component_m_mk="/codecs_v2/omx/omx_m4venc/build/make_multithreaded"
SOLIBDIRS_omx_m4vdec_sharedlibrary="/codecs_v2/omx/omx_m4v/build/make_multithreaded /codecs_v2/video/m4v_h263/dec/build/make"
pvcommsionode_m_mk="/nodes/pvcommsionode/build/make"
pvmp4reginterface_m_mk="/modules/linux_mp4/node_registry/build/make"
pvgeneraltools_m_mk="/protocols/systems/tools/general/build/make"
unit_test_m_mk=""
omx_m4venc_component_imp_m_lib=""
pvomxaudiodecnode_m_mk="/nodes/pvomxaudiodecnode/build/make"
pvjanusplugininterface_m_lib=""
SOLIBS_omx_avcenc_sharedlibrary="-lomx_avcenc_component_lib -lpvavch264enc"
pvvideodecnode_m_lib=""
getactualaacconfig_imp_m_mk="/codecs_v2/audio/aac/dec/util/getactualaacconfig/build/make"
pvrmffrecognizer_y_lib=""
pvamrffrecognizer_y_lib=""
omx_common_m_mk="/codecs_v2/omx/omx_common/build/make_multithreaded"
SOLIBDIRS_omx_mp3dec_sharedlibrary="/codecs_v2/omx/omx_mp3/build/make_multithreaded /codecs_v2/audio/mp3/dec/build/make"
pvasfffparsernode_m_mk=""
LIBS_omxjointimp_shared="-lomx_mastercore_lib"
pvlatmpayloadparser_m_mk="/protocols/rtp_payload_parser/util/build/latmparser/make"
pvrmffparsernode_y_mk=""
getactualaacconfig_for_static_m_mk=""
pvdbmanager_m_lib=""
MODS_pvwmdrm="-lopencore_player -lopencore_common"
wmvdecoder_imp_m_mk="n"
pvamrwbdecoder_m_mk="/codecs_v2/audio/gsm_amr/amr_wb/dec/build/make"
pvrtsp_cli_eng_node_opencore_m_mk="/protocols/rtsp_client_engine/build_opencore/make"
mshttp_support_mk=""
opencore_download_so_name="opencore_download"
SOLIBS_pvmtpdb="  "
SOLIBS_opencore_downloadreg="-lpvdownloadreginterface"
mp3decnode_m_mk=""
SOLIBS_omx_sharedlibrary="-lomx_common_lib -lomx_queue_lib -lpvomx_proxy_lib -lomx_baseclass_lib -lpv_omx_interface"
pvwavffrecognizer_y_lib=""
pvavcencnode_m_lib=""
pvjitterbuffernode_y_lib=""
pvmediadatastruct_m_mk="/baselibs/media_data_structures/build/make"
REGISTER_OMX_M4VENC_COMPONENT=1
LIBDIR_fileformats_static="                   "
pvpvrff_y_mk=""
pvpvrnode_y_mk=""
MODS_omx_amrenc_sharedlibrary="-lomx_sharedlibrary -lopencore_common "
LIBS_cpm_shared="-lcpm -lpassthru_oma1   "
pvmediainputnode_y_lib=""
pvmiofileinput_y_mk=""
pvmiofileoutput_m_lib="-lpvmiofileoutput"
pvmp4ffparsernode_m_lib=""
omx_m4venc_component_y_mk=""
LIBDIR_omxdecimp_static=""
omx_amr_component_y_lib=""
rvdecoder_y_lib=""
LIBS_pvmi_static="                     "
pvplayer_engine_y_mk=""
MODS_opencore_common=""
pvsqlite_m_lib=""
pvmp4ffopencore_m_mk="/fileformats/mp4/parser/build_opencore/make"
pv_amr_nb_common_lib_m_mk="/codecs_v2/audio/gsm_amr/amr_nb/common/build/make"
LIBDIR_tools_v2_shared="/modules/linux_rtsp/core/build/make /modules/linux_rtsp/node_registry/build/make   /modules/linux_download/core/build/make /modules/linux_download/node_registry/build/make /modules/linux_mp4/core/build/make /modules/linux_mp4/node_registry/build/make  "
DYNAMIC_LOAD_OMX_AMRENC_COMPONENT=1
pvamrencnode_y_lib=""
pvjanusplugin_y_mk=""
pv324m_m_lib="-lpv324m"
pvvideoencnode_m_mk=""
LIBDIR_nodes_static="                                                  "
LIBDIR_audio_shared="  /codecs_v2/audio/aac/dec/build/make /codecs_v2/audio/gsm_amr/amr_nb/common/build/make /codecs_v2/audio/gsm_amr/amr_wb/dec/build/make /codecs_v2/audio/gsm_amr/amr_nb/dec/build/make /codecs_v2/audio/mp3/dec/build/make  /codecs_v2/audio/gsm_amr/common/dec/build/make /codecs_v2/audio/gsm_amr/amr_nb/enc/build/make"
SOLIBDIRS_pvwmdrmdev="n"
opencore_mp4local_so_name="opencore_mp4local"
pvavcdecnode_y_lib=""
threadsafe_callback_ao_y_lib=""
pvmimeutils_m_mk="/baselibs/pv_mime_utils/build/make"
pvmp4ffcomposernodeopencore_y_lib=""
pvavcdecnode_y_mk=""
pvjitterbuffernode_opencore_y_lib=""
rdt_parser_m_mk=""
omx_baseclass_y_mk=""
pvmp4ffcomposeropencore_y_lib=""
pvfileparserutils_m_lib="-lpvfileparserutils"
pv_http_parcom_y_lib=""
pvgendatastruct_m_lib="-lpvgendatastruct"
omx_wmv_component_imp_m_lib="n"
SOLIBS_opencore_rtspreg="-lpvrtspreginterface"
protocolenginenode_m_mk=""
pvmedialayernode_y_lib=""
pvdivxffrecognizer_m_mk=""
SOLIBDIRS_omx_avcenc_sharedlibrary="/codecs_v2/omx/omx_h264enc/build/make_multithreaded /codecs_v2/video/avc_h264/enc/build/make"
pvdownloadmanagernode_m_mk="/nodes/pvdownloadmanagernode/build/make"
pvid3parcom_y_mk=""
pvstreamingmanagernode_m_mk=""
#
# That's all, folks!
