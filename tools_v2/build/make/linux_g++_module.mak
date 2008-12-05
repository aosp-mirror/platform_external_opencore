#
# This makefile template should be included by makefiles to
# build modules and uses the g++ compiler.
# See the file module.mk for more details.
#

BUILD_ARCH = $(ARCHITECTURE)

override CXX = g++

SHARED_LINK = g++
SHARED_LDFLAGS = -shared -Wl,-Bsymbolic --no-undefined -Wl,--whole-archive
SONAME_ARG = -Wl-soname,$(SONAME) 
POST_SHARED_LDFLAGS = -Wl,--no-whole-archive

include $(MK)/module.mk
