include $(PV_TOP)/makefile.pv

WHAT = module
TARGET = pvauthor

export FORMAT=nj
LIBDIRS = \
	$(VOB_BASE_DIR)/engines/author/build/make \
	$(VOB_BASE_DIR)/codecs_v2/video/m4v_h263/enc/build/make \
	$(VOB_BASE_DIR)/codecs_v2/audio/gsm_amr/amr_nb/enc/build/make \
	$(VOB_BASE_DIR)/codecs_v2/video/avc_h264/enc/build/make \
	$(VOB_BASE_DIR)/fileformats/mp4/composer/build_opencore/make \
	$(VOB_BASE_DIR)/nodes/pvamrencnode/build/make \
	$(VOB_BASE_DIR)/nodes/pvmp4ffcomposernode/build_opencore/make \
	$(VOB_BASE_DIR)/nodes/pvvideoencnode/build/make \
	$(VOB_BASE_DIR)/nodes/pvavcencnode/build/make \
	$(VOB_BASE_DIR)/nodes/pvmediainputnode/build/make_pvauthor \
	$(VOB_BASE_DIR)/pvmi/media_io/pvmi_mio_fileinput/build/make_pvauthor \
	$(VOB_BASE_DIR)/pvmi/media_io/pvmi_mio_avi_wav_fileinput/build/make \
	$(VOB_BASE_DIR)/fileformats/avi/parser/build/make \


LIBS = -lpvauthorengine -lpvamrencnode -lpvavcencnode -lpvavch264enc \
	   -lpvencoder_gsmamr -lpvm4vencoder -lpvmp4ffcomposer \
	   -lpvmp4ffcomposernode -lpvvideoencnode -lpvmediainputnode \
	   -lpvmiofileinput -lpvmioaviwavfileinput -lpvavifileparser -lpvwav \
	   
	   

# use --allow-multiple-definition to get around aac decoder and
# getactualaacconfig building the same code
XLDFLAGS += -Wl,--allow-multiple-definition

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template
