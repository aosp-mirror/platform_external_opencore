include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = unit_test

# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = " 
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS += 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.
# See notes above regarding overriding project defaults.
#
# XCXXFLAGS += 

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
# XINCDIRS += 


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

SRCS =  test_case.cpp \
	test_problem.cpp \
	test_result.cpp \
	text_test_interpreter.cpp \
        unit_test_main.cpp \
	unit_test_args.cpp

HDRS =  stringable.h \
	test_case.h \
	test_problem.h \
	test_result.h \
	text_test_interpreter.h \
	unit_test_common.h \
	unit_test_local_string.h \
	unit_test_args.h \
	unit_test_vector.h 



DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template


