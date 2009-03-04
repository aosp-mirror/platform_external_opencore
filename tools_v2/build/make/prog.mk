#
# This makefile is included by language- or compiler-specific makefiles.
# The makefiles including those may define LIBS to a list
# of libraries that this program needs to link with.  This LIBS should
# have syntax suitable for being included on the link commandline.
# See also the rules.mk file for descriptions of meanings of
# other variables.
#

# if DBG_SUFFIX not set then set to _debug
ifeq ($(strip $(DBG_SUFFIX)),)
  DBG_SUFFIX = _debug
endif


TARGETS_TO_INSTALL+=default

ifneq ($(strip $(RSRCS)),)
  ifneq ($(strip $(RSRCDESTDIR)),)
    TARGETS_TO_INSTALL+=resources
  endif
endif

resources_srcdir=.
resources_destdir=$(RSRCDESTDIR)
resources_files=$(RSRCS)
resources_ifunction=_resource_install_

define _resource_install_
	$(INSTALL) -c -m 444 WHAT_TO_INSTALL $(resources_destdir)
endef

OBJDIRS = $(BUILD_ARCH)

OBJS = $(COMPILED_OBJS:%.$(STAT_OBJS_EXT)=$(OBJDIRS)/%.$(STAT_OBJS_EXT))

DEPS = $(OBJS:%.$(STAT_OBJS_EXT)=%.d)

CLEAN += $(DEPS)

include $(MK)/rules.mk

# Note that clearmake does not appear to handle vpath correctly
# If a system file only has a shared version then it complains that
# it can't find the dependency (if the library is listed in the 
# dependency list).  The work-around for now is to include the 
# library in the XLDFLAGS.  One example is libpthread on the Suns.

ifeq ($(HOST_ARCH), win32)
  vpath %.so $(LIB_DIRS:$(LIBCOMPFLAG)%=%)
  vpath %.$(STAT_LIB_EXT) $(LIB_DIRS:$(LIBCOMPFLAG)%=%)
else
  vpath lib%.so $(LIB_DIRS:$(LIBCOMPFLAG)%=%)
  vpath lib%.$(STAT_LIB_EXT) $(LIB_DIRS:$(LIBCOMPFLAG)%=%)
endif

ifneq ($(HOST_ARCH), win32)
define group_writable_target
	chmod g+w $(REALTARGET)
endef
endif


ifeq ($(TOOLSET), cl)
define build_target
	-$(RM) $(REALTARGET)
		_LINK_ $(BINDING) /out:$(REALTARGET) $(patsubst -L%,/libpath:%,$(filter-out -l%,$(LDFLAGS))) \
			$(OBJS)	$(XOBJECTS) $(patsubst -l%,%.$(STAT_LIB_EXT),$(filter -l%,$(LDFLAGS))) \
			$(POST_LDFLAGS)
		$(rm_vc60)
	$(group_writable_target)
endef

else
define build_target
	-$(RM) $(REALTARGET)
		_LINK_ $(BINDING) -o $(REALTARGET)  $(filter-out -l%,$(LDFLAGS)) \
			$(PRE_LDFLAGS) $(OBJS) $(XOBJECTS) $(filter -l%,$(LDFLAGS)) $(POST_LDFLAGS)
	$(group_writable_target)
endef
endif


static::
	-$(RM) $(REALTARGET)
	$(MAKE) $(MFLAGS) $(REALTARGET) BINDING=$(STATIC_BINDING)

link::
	-$(RM) $(REALTARGET)
	$(MAKE) $(MFLAGS) $(REALTARGET)

ifeq ($(strip $(TEST_PROG)),)
  ifneq ($(HOST_ARCH), win32)
    TEST_PROG = $(REALTARGET)
  else
    # Use DOS path delimeter to run executable
    TEST_PROG = $(subst /,\,$(REALTARGET))
  endif
endif

ifneq ($(strip $(TEST_PROG)),NOTEST)
run_test::	$(REALTARGET)
	$(TEST_PROG) $(TEST_ARGS) $(SOURCE_ARGS)
endif

.PRECIOUS:: $(OBJS)
