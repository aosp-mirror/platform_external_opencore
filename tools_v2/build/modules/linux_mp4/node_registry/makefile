# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = module
TARGET = pvmp4reg

LIBDIRS = \
	$(VOB_BASE_DIR)/tools_v2/build/modules/linux_mp4/node_registry/src \
	$(VOB_BASE_DIR)/fileformats/mp4/parser/build/make \
	$(VOB_BASE_DIR)/pvmi/recognizer/plugins/pvmp4ffrecognizer/build/make \

LIBS = -lpvmp4reginterface -lpvmp4ff -lpvmp4ffrecognizer

MODS = -lpvcommon -lpvplayer

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

