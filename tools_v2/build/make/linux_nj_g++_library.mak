#
# This makefile template should be included by makefiles in
# library directories which use the g++ compiler.
# See the file linux_arm_library.mk for more details.
#

BUILD_ARCH = $(ARCHITECTURE)

override CXX = arm-eabi-g++

SHARED_LINK = arm-eabi-g++
SHARED_CFLAGS = -fPIC
SHARED_CXXFLAGS = -fPIC
SHARED_LDFLAGS = -shared

SONAME_ARG = -Wl,-h,$(SONAME)

#Make all warnings into errors.
FLAG_COMPILE_WARNINGS_AS_ERRORS = -Werror

include $(MK)/linux_nj_library.mk
