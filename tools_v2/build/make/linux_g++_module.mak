#
# This makefile template should be included by makefiles to
# build modules and uses the g++ compiler.
# See the file module.mk for more details.
#

BUILD_ARCH = $(ARCHITECTURE)

override CXX = g++

SHARED_LINK = g++
SHARED_LDFLAGS = -shared
SONAME_ARG = -nostdlib -Wl-soname,$(SONAME) -Wl,-shared,-Bsymbolic --no-undefined -Wl,--whole-archive

include $(MK)/module.mk
