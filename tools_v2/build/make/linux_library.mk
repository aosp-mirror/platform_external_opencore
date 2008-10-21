#
# This makefile template should be included by makefiles in
# library directories.  The including makefile may define the
# HDRS variable to a list of header files to get installed.
# See also the rules.mk file for descriptions of meanings of
# other variables.
#

BUILD_ARCH = $(ARCHITECTURE)

STAT_LIB_EXT = a
STAT_OBJS_EXT = o

include $(MK)/library.mk

