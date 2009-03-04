# 
# This makefile template is included by all other rules files.  It
# defines the default target of all, the global clean rule, and
# takes care of defining other global parameters for all makes.
#

#
# Get the local configuration
#
include $(MK)/local.mk

#
# Process the src list
#
include $(MK)/srclist.mk

#
# If BUILD_ARCH has anything in it, include the file for that architecture
#
ifneq ($(strip $(BUILD_ARCH)),)
  -include $(MK)/$(BUILD_ARCH).mk
endif

#
# If a target name hasn't been defined, assume it's tge same as
# the current directory.
#
ifndef TARGET
TARGET=$(notdir $(shell $(MK)/../bin/cc_pwd))
endif

#
# The default rule
#
default: all

#
# Alternate rules for when things are not otherwise defined
#
stage::

print_dirs::
	@$(MK)/../bin/cc_pwd


show-doc::
	@$(SHOW_DOC) $(GNU_MAKE_TEMPLATE)
