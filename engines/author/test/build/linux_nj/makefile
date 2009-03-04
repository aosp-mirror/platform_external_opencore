# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = prog
TARGET = test_pvauthorengine

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
#
# XCPPFLAGS +=  

# Use XINCDIRS for extra include directories. These should be proceeded 
# with "-I" just as they would be when passing to the compiler.
#
# Added this to for osclconfig.h
#XINCDIRS += -I../../../config/$(BUILD_ARCH)  -I../../../config/shared

# include the common engine header files in the search path
XINCDIRS += -I ../../../../common/include -I ../../config/linux_nj -I../../src/single_core -I../../../../../pvmi/pvmf/include -I../../../../../nodes/common/include -I../../../../../pvmi/media_io/pvmi_mio_fileinput/include -I../../../../../pvmi/media_io/pvmi_mio_fileinput/src -I../../../../../pvmi/media_io/pvmi_mio_avi_wav_fileinput/include -I../../../../../fileformats/avi/parser/include

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
SRCS =	pvaetest.cpp test_pv_author_engine_testset1.cpp test_pv_author_engine_testset2.cpp test_pv_author_engine_testset3.cpp test_pv_author_engine_testset4.cpp ./single_core/pvaetestinput.cpp test_pv_author_engine_testset5.cpp test_pv_author_engine_testset6.cpp test_pv_author_engine_testset7.cpp test_pv_mediainput_author_engine.cpp


#to use the _mod versions because the project is built with the LIBTARGET=module-rel option
LIBS = \
-lpvcommon \
-lpvauthor \
-lunit_test_mod \
-lpvwav_mod \
-lpvmiofileinput_mod \
-lpvmioaviwavfileinput_mod \
-lpvavifileparser_mod \
-lpvfileparserutils_mod 


SYSLIBS = -lpthread -ldl

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

