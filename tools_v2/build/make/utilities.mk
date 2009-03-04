#

firstrule:: all

# ----------------------------------------------------------------------
#		     DEFINITIONS USED INTERNALLY
# ----------------------------------------------------------------------

# If INSTALL_PREFIX has been defined, we're 

ifneq ($(INSTALL_PREFIX),)
  INSTALL_SRCDIR=$($(INSTALL_PREFIX)_srcdir)
  INSTALL_DESTDIR=$($(INSTALL_PREFIX)_destdir)
  INSTALL_FILES=$($(INSTALL_PREFIX)_files)
  INSTALL_IFUNCTION=$($(INSTALL_PREFIX)_ifunction)
  INSTALL_TARGETS=$(addprefix $(INSTALL_DESTDIR)/,$(INSTALL_FILES))

  foo::
	@echo INSTALL_SRCDIR=$(INSTALL_SRCDIR)
	@echo INSTALL_DESTDIR=$(INSTALL_DESTDIR)
	@echo INSTALL_FILES=$(INSTALL_FILES)
	@echo INSTALL_IFUNCTION=$(INSTALL_IFUNCTION)
	@echo INSTALL_TARGETS=$(INSTALL_TARGETS)
endif



REAL_DEST=$(INSTALL_DESTDIR)


# This target syncs the staging area if something to do so has been
# defined.  If NOSYNC is defined, skip the whole thing but emit a
# warning.

.PHONY:: sync-up

ifndef NOSYNC
    sync-up::
	$(_sync_staging_)
else
    sync-up::
	@echo "(WARNING: Not syncing staging area.  This could be risky.)"
endif


# ----------------------------------------------------------------------
#				SUBDIR
# ----------------------------------------------------------------------

#
# Make a target in another directory.  The basic format for
# triggering this dependency is dir-target-subdir, where dir
# is the name of the subdirectory, target is the name of the
# target to make and subdir is the word "subdir."  For example:
#
#	LIST=foo bar baz
#	foobarbaz:: $(LIST:%=src/%-clean-subdir)
#
# will cause the execution of
#
#	make -C foo clean
#	make -C bar clean
#	make -C baz clean
#

ifeq ($(NOSUBDIRS),)
%-subdir: sync-up
	$(_stage_subdir_)
	@$(MAKE) $(MFLAGS) -C $(subst -, , $(subst -subdir,,$@))
else
%-subdir:
	@true
endif


# ----------------------------------------------------------------------
#			  INSTALL/UNINSTALL
# ----------------------------------------------------------------------

#
# By default, the "install" target will place a copy of $(REALTARGET) in
# a $(DESTDIR), which will be staged if you're in a staging area.
# 
# TARGETS_TO_INSTALL - A list of what should be installed.  For example,
# if you're building a library that installs both the default target and
# some includes, set this variable to "default includes".  This will do
# the default installation plus one named "includes."  Set this to N-O-N-E
# in templates that are just waypoints and don't install anything.
# 
# xxx_srcdir - Name of the directory where files to be installed can be
# found.  This defaults to "./." if none other is specified.
#
# xxx_destdir - The directory where target "xxx" is installed, where
# "xxx" is the name of a target you defined in $(TARGETS_TO_INSTALL).
#
# xxx_files - A list of the files that should be installed, where
# "xxx" is the name of a target you defined in $(TARGETS_TO_INSTALL).
# 
# xxx_ifunction - This is the name of a canned function that does the
# installation of a single file, where "xxx" is the name of a target
# you defined in $(TARGETS_TO_INSTALL).  The function should be set up
# such that the word WHAT_TO_INSTALL may be replaced with a list of
# one or more files to install.  The macro $(DESTDIR) will indicate where
# the files should be installed.  If you want to use the default installation
# method, set this to _default_install_.
#

#
# This is the default installation method, used if none other is
# specified.
#

ifeq ($(TARGETS_TO_INSTALL),)
  TARGETS_TO_INSTALL = default
else
  ifeq ($(TARGETS_TO_INSTALL),N-O-N-E)
    TARGETS_TO_INSTALL=
  endif
endif

# The default installation target

default_srcdir = $(patsubst %/,%,$(dir $(REALTARGET)))
default_destdir = $(DESTDIR)
default_files = $(notdir $(REALTARGET))
default_ifunction=_default_install_
define _default_install_
  $(INSTALL) -c -m 554 WHAT_TO_INSTALL $(INSTALL_DESTDIR)
endef

# This is the actual "install" target, which builds everything, takes
# whatever's in $(TARGETS_TO_INSTALL) and creates dependencies on the
# generalized %-install rule, which actually does the dirty work.

.PHONY:: install

install:: all sync-up $(addsuffix -install,$(TARGETS_TO_INSTALL))

#
# This is the generalized installation rule.  If INSTALL_PREFIX has not
# been defined, do a recursive make of the target with INSTALL_PREFIX set
# to the target name.  This makes INSTALL_xxx point at the proper rules.
# Otherwise, install each file individually.
#

ifeq ($(INSTALL_PREFIX),)

  %-install::
	@$(MAKE) $(MFLAGS) NODEPS=1 \
		INSTALL_PREFIX=$(firstword $(subst -, ,$@)) \
		make-and-stage $(IMPORTANT_FILE) \
		$(firstword $(subst -, ,$@))-install

else

  .PHONY:: $(INSTALL_PREFIX)-install

  # Install files only if the destination directory isn't "."

  ifneq ($(INSTALL_DESTDIR),.)

    INSTALLS=$(addprefix $(INSTALL_DESTDIR)/,$(INSTALL_FILES))


    $(INSTALL_PREFIX)-install:: $(INSTALLS)

    # This removes a file if it is a symbolic link.

    %-unlink::
	@if test -h $(INSTALL_DESTDIR)/$(firstword $(subst -, ,$@)) ; \
		then \
			$(RM) $(INSTALL_DESTDIR)/$(firstword $(subst -, ,$@)) ; \
		fi
  else

    $(INSTALL_PREFIX)-install::
	@echo "(Destination directory is . -- Install not done.)"

  endif

endif


#
# This dependency installs a single file if the original is newer than
# the installed version.
#

ifneq ($(INSTALL_DESTDIR),)
$(INSTALL_DESTDIR)/%: $(INSTALL_SRCDIR)/%
	$($(INSTALL_IFUNCTION):WHAT_TO_INSTALL=$<)
endif


#
# This target creates $(REAL_DEST) and, if we're using a baseline control
# system, stages it.
#

.PHONY:: make-and-stage

ifneq ($(INSTALL_DESTDIR),.)
  make-and-stage::
	$(MK)/../bin/cc_mkdir $(REAL_DEST)
	$(_stage_destdir_)
else
  make-and-stage::
	@true
endif


#
# Uninstall works a lot like install, except that it removes the installed
# files and does nothing else.
#
.PHONY:: uninstall stageall

stageall::
	$(_stage_all_destdirs_)

uninstall:: stageall $(addsuffix -uninstall,$(TARGETS_TO_INSTALL))

%-uninstall::
	$(RM) $(foreach FILE,$($(firstword $(subst -, ,$@))_files),\
		$($(firstword $(subst -, ,$@))_destdir)/$(FILE))


# ----------------------------------------------------------------------
#				CLEAN
# ----------------------------------------------------------------------

#
# clean - Wipe out anything that can be re-made.  If CLEAN_FILES is not
# defined, we have an older, non-fully-defined template.  Otherwise, fall
# back on the previous method.
#
ifneq ($(CLEAN),)
clean::
	-$(RM) $(ARG_QUOTES)$(REALTARGET) $(CLEAN_DEFAULTS) $(CLEAN) $(XCLEAN)$(ARG_QUOTES)

else
clean::
	-$(RM) $(ARG_QUOTES)$(REALTARGET) $(CLEAN_DEFAULTS) *.[od] $(OBJDIR)/*.[od] *.tmp* *.obj$(ARG_QUOTES)
endif

