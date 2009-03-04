# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = module
TARGET = pvnet_support

LIBDIRS = $(VOB_BASE_DIR)/nodes/pvprotocolenginenode/build/make \
		  $(VOB_BASE_DIR)/nodes/pvsocketnode/build/make \
		  $(VOB_BASE_DIR)/protocols/http_parcom/build/make \
		  $(VOB_BASE_DIR)/baselibs/gen_data_structures/build/make \
		  $(VOB_BASE_DIR)/protocols/sdp/parser/build/make

LIBS = -lprotocolenginenode -lpvsocketnode -lpv_http_parcom -lpvgendatastruct -lpvsdpparser

MODS = -lpvcommon -lpvplayer

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

