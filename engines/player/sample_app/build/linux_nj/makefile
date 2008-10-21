# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = prog
TARGET = sample_player_app

# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = " 
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS += 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.
# See notes above regarding overriding project defaults.
#
# XCXXFLAGS += -DUSE_UNICODE  -D_POSIX_PTHREAD_SEMANTICS   #-D_POSIX_SOURCE

# Use XCPPFLAGS to add extra preprocessor directives. Use XINCDIRS
# for other include directories, but please only do this if absolutely
# necessary -- let the makefile system find the include directories if 
# possible.
# See notes above regarding overriding project defaults.
#XCPPFLAGS += 

# Use XINCDIRS for extra include directories. These should be proceeded 
# with "-I" just as they would be when passing to the compiler.
#
# Added this to for osclconfig.h
#XINCDIRS += -I../../../config/$(BUILD_ARCH)  -I../../../config/shared

# include the common engine header files in the search path
XINCDIRS += -I ../../src/linux_nj -I ../../../../common/include -I ../../../../../pvmi/media_io/pvmiofileoutput/include -I ../../../../../nodes/pvmediaoutputnode/include -I ../../../include -I ../../../../../nodes/common/include

ifeq ($(ARCHITECTURE),linux_nj)
   XINCDIRS += -I ../../config/$(BUILD_ARCH)
else
   XINCDIRS += -I ../../config/linux_3gpp
endif


# Use XLIBDIRS for extra library directories. These should be proceeded 
# with "-L" just as they would be when passing to the linker. 
# Used for building executables.


# Use XLDFLAGS for extra linker directives.
# Used for building executables.
#
#XLDFLAGS +=

SRCDIR = ../../src
INCSRCDIR = ../../src

# compose final src list for actual build
SRCS =	sample_player_app.cpp pv_player_engine_testset_mio_file.cpp

LIBS = \
	-lpvplayer_engine \
	-lpvfileoutputnode \
	-lpvpvxparser \
	-lpvmp3ffparsernode \
	-lpvdownloadmanagernode \
	-lpvmp4ffparsernode \
	-lcpm \
	-lpassthru_oma1 \
	-lpvaacffparsernode \
	-lpvmp3ff \
	-lpvmp4ff \
	-lpvaacparser \
	-lgetactualaacconfig \
	-lpvmp3 \
        -lm4v_config \
	-lpvlogger \
	-lpvvideodecnode \
	-lpvavcdecnode \
	-lpvstreamingmanagernode_3gpp \
	-lpvjitterbuffernode \
	-lpvmediaplayernode \
	-lrtprtcp \
	-lprotocolenginenode \
	-lpvrtsp_cli_eng_node \
	-lpvmediadatastruct \
	-lpv_http_parcom \
	-lpvsocketnode \
	-lrtppayloadparser_3gpp \
	-lpvbitstreamparser \
	-lpvmf \
	-lpvsdpparser \
	-lpv_rtsp_parcom \
	-lpvcrypto \
	-lpvmp4decoder \
	-lpvdecoder_gsmamr \
	-lpv_amr_nb_common_lib \
	-lpvamrwbdecoder \
	-lpvavcdecoder \
	-lpv_avc_common_lib \
	-lpv_aac_dec \
	-lfile_mediaio \
	-lpvgendatastruct \
	-lpvmimeutils \
	-lpvmediaoutputnode \
	-lpvamrffparsernode \
	-lpvgsmamrparser \
	-lpvlatmpayloadparser \
	-lpvmfrecognizer \
	-lpvmp3ffrecognizer \
	-lpvmp4ffrecognizer \
	-lpvfileparserutils \
	-lpvid3parcom \
	-losclregcli \
	-losclregserv \
	-losclio \
	-losclproc \
	-losclutil \
	-losclmemory \
	-losclerror \
	-losclbase \
	-lunit_test \

ifeq ($(ARCHITECTURE),linux_nj)
    SYSLIBS = -lc -ldl -lstdc++
    PRE_LDFLAGS += \
        $(ANDROID_BASE)/../crtbegin.o
    POST_LDFLAGS += \
        $(ANDROID_BASE)/../toolchain-4.1.1/bin/../lib/gcc/arm-elf/4.1.1/libgcc.a \
        $(ANDROID_BASE)/../toolchain-4.1.1/bin/../lib/gcc/arm-elf/4.1.1/crtend.o
else
    SYSLIBS = -lpthread
endif


DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

