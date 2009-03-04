#
# This makefile template should be included by makefiles in
# library directories which use the gcc compiler.
# See the file linux_library.mk for more details.
#

BUILD_ARCH = $(ARCHITECTURE)

override CC = gcc

SHARED_LINK = gcc
SHARED_CFLAGS = -fPIC
SHARED_CXXFLAGS = -fPIC
SHARED_LDFLAGS = -shared

SONAME_ARG = -Wl,-h,$(SONAME)


include $(MK)/linux_library.mk
