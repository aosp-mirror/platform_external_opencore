# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvmediadatastruct

# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = " 
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS += 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.
# See notes above regarding overriding project defaults.
#
#XCXXFLAGS += -DTRUE=true -DFALSE=false
XCXXFLAGS += $(FLAG_COMPILE_WARNINGS_AS_ERRORS)

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
XINCDIRS += -I../../../../fileformats/mp4/parser/include
XINCDIRS += -I../../../../pvmi/pvmf/include


# Use XLIBDIRS for extra library directories. These should be proceeded 
# with "-L" just as they would be when passing to the linker. 
# Used for building executables.
#
# XLIBDIRS += 

# Use XLDFLAGS for extra linker directives.
# Used for building executables.
#
# XLDFLAGS += 

SRCDIR = ../../src
INCSRCDIR = ../../src

# compose final src list for actual build
SRCS =	media_clock_converter.cpp \
	pv_gau.cpp \
	access_unit.cpp \
	access_unit_impl.cpp \
	time_comparison_utils.cpp \
	au_utils.cpp
	


HDRS =  access_unit.h \
	access_unit_impl.h \
	au_utils.h \
	gau_object.h \
	media_clock_converter.h \
	time_comparison_utils.h \
	pv_gau.h


DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

