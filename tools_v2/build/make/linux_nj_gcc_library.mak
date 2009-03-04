#
# This makefile template should be included by makefiles in
# library directories which use the gcc compiler.
# See the file linux_arm_library.mk for more details.
#

BUILD_ARCH = $(ARCHITECTURE)

override CC = arm-eabi-gcc

SHARED_LINK = arm-eabi-gcc
SHARED_CFLAGS = -fPIC
SHARED_CXXFLAGS = -fPIC
SHARED_LDFLAGS = -shared

SONAME_ARG = -Wl,-h,$(SONAME)


include $(MK)/linux_nj_library.mk

