# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = prog
TARGET = pvplayer_engine_test

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

XCXXFLAGS += -DBUILD_OMX_DEC_NODE

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
XINCDIRS += -I ../../../../common/include -I ../../../../../pvmi/media_io/pvmiofileoutput/include -I ../../../../../nodes/pvmediaoutputnode/include -I ../../../include -I ../../../../../nodes/common/include -I ../../../../../extern_libs_v2/khronos/openmax/include

ifeq ($(ARCHITECTURE),linux_arm)
   XINCDIRS += -I ../../config/$(BUILD_ARCH)
else
   XINCDIRS += -I ../../config/linux_nj
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
SRCS =  test_pv_player_engine.cpp \
    test_pv_player_engine_testset_mio_file.cpp \
        test_pv_player_engine_testset1.cpp \
        test_pv_player_engine_testset5.cpp \
        test_pv_player_engine_testset6.cpp \
        test_pv_player_engine_testset7.cpp \
        test_pv_player_engine_testset8.cpp \
        test_pv_player_engine_testset9.cpp \
        test_pv_player_engine_testset10.cpp \
        test_pv_player_engine_testset11.cpp \
        test_pv_player_engine_testset12.cpp \
        test_pv_player_engine_testset13.cpp \
	test_pv_player_engine_testset_cpmdlapassthru.cpp

# Need to use the _mod versions because the project is built with the LIBTARGET=module-rel option
LIBS = -lunit_test_mod -lpvcommon -lpvplayer

#MODS = -lpvcommon -lpvplayer

#XLDFLAGS += -lpvcommon_debug -lpvplayer_debug

SYSLIBS = -lpthread -ldl

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

