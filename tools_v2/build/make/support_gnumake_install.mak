#
#BEGINDOC
# -----------------------------------------------------------------------------
#
# This GNUmake template installs targets such as shell scripts and common
# include files that don't need to be built, just installed.
#
# SUPPORTED MACROS:
#
# DESTDIR	Where the installed file(s) should go
#
# SRCS		A list of the files to be installed.
#
# MODE		The permissions for each installed file.  By default this
#		is 664.
# 
# -----------------------------------------------------------------------------
#ENDOC

ifeq ($(MODE),)
  MODE=444
endif

TARGETS_TO_INSTALL=generic

generic_srcdir=.
generic_destdir=$(DESTDIR)
generic_files=$(SRCS)
generic_ifunction=_generic_install_

define _generic_install_
	install -c -m $(MODE) WHAT_TO_INSTALL $(generic_destdir)
	$(RANDOMLIB)
endef


include $(MK)/rules.mk
