#
# This makefile template should be included by makefiles in
# library directories.  The including makefile may define the
# HDRS variable to a list of header files to get installed.
# See also the rules.mk file for descriptions of meanings of
# other variables.
#


# If we have headers, we should install those, too. 
# The only exception is if the NOHDRINST macro is defined.
# The NOHDRINST macro is used to bypass the headers-install
# build step.  It is useful for situations where all the header
# files are already visible within the compiler search path.
# ONLY DEFINE THE MACRO IF YOU KNOW WHAT YOU'RE DOING
ifndef NOHDRINST
  ifneq ($(HDRS),)
    ifneq ($(INCDESTDIR),)
      TARGETS_TO_INSTALL+=headers
    endif
  endif
endif

ifneq ($(INCSRCDIR),)
   headers_srcdir=$(INCSRCDIR)
else
   headers_srcdir=.
endif
headers_destdir=$(INCDESTDIR)
headers_files=$(HDRS)
headers_ifunction=_header_install_

define _header_install_
	$(INSTALL) -c -m 444 WHAT_TO_INSTALL $(headers_destdir)
endef


# if TARGET is empty then just do the headers install
ifeq ($(strip $(TARGET)),)

include $(MK)/rules.mk

library-install:

cm-release:

.PHONY:: cm-release library-install
else

# if DBG_SUFFIX not set then set to _debug
ifeq ($(strip $(DBG_SUFFIX)),)
  DBG_SUFFIX = _debug
endif

ifeq ($(RELEASE),1)
  # if LIBTARGET not set then set to 
  ifeq ($(strip $(LIBTARGET)),)
    LIBTARGET = $(STAT_REL_LIB)
  endif
else 
  ifeq ($(strip $(LIBTARGET)),)
    LIBTARGET = $(STAT_DBG_LIB)
  endif
endif

ifneq ($(word 2,$(strip $(LIBTARGET))),)

# there is more than one target listed in libtarget so 
# go through them one-by-one

#echo "running make with target $(target)"; echo ""
#$(shell $(MAKE) LIBTARGET=$(target))
define single_target_make
$(shell echo "$(target)")
endef

default:
	@export MAKEFLAGS
	$(MK)/../bin/make_list	$(MAKE) $@ LIBTARGET $(LIBTARGET)

%: 
	@export MAKEFLAGS
	$(MK)/../bin/make_list $(MAKE) $@ LIBTARGET $(LIBTARGET)


else


ifneq ($(strip $(MAJOR_VERSION)),)
  SONAME_EXT = .$(MAJOR_VERSION)
  ifneq ($(strip $(MINOR_VERSION)),)
      SHAR_LIB_EXT = .$(MAJOR_VERSION).$(MINOR_VERSION)
  else
      SHAR_LIB_EXT = .$(MAJOR_VERSION)
  endif
else
  SHAR_LIB_EXT = 
endif

MY_TARGET = $(strip $(TARGET))

STAT_REL_OBJDIR = $(BUILD_ARCH)/static_rel
STAT_DBG_OBJDIR = $(BUILD_ARCH)/static_dbg
SHAR_REL_OBJDIR = $(BUILD_ARCH)/shared_rel
SHAR_DBG_OBJDIR = $(BUILD_ARCH)/shared_dbg
MODL_REL_OBJDIR = $(BUILD_ARCH)/module_rel
MODL_DBG_OBJDIR = $(BUILD_ARCH)/module_dbg

OBJDIRS = $(STAT_REL_OBJDIR) $(STAT_DBG_OBJDIR) $(SHAR_REL_OBJDIR) $(SHAR_DBG_OBJDIR) $(MODL_REL_OBJDIR) $(MODL_DBG_OBJDIR)

ifeq ($(BUILD_ARCH),win32)
  STAT_REL_LIB = $(MY_TARGET:%=$(STAT_REL_OBJDIR)/%.$(STAT_LIB_EXT))
  STAT_DBG_LIB = $(MY_TARGET:%=$(STAT_DBG_OBJDIR)/%$(DBG_SUFFIX).$(STAT_LIB_EXT))
  SHAR_REL_LIB = $(MY_TARGET:%=$(SHAR_REL_OBJDIR)/%.so$(SHAR_LIB_EXT))
  SHAR_DBG_LIB = $(MY_TARGET:%=$(SHAR_DBG_OBJDIR)/%$(DBG_SUFFIX).so$(SHAR_LIB_EXT))
else
  STAT_REL_LIB = $(MY_TARGET:%=$(STAT_REL_OBJDIR)/lib%.$(STAT_LIB_EXT))
  STAT_DBG_LIB = $(MY_TARGET:%=$(STAT_DBG_OBJDIR)/lib%$(DBG_SUFFIX).$(STAT_LIB_EXT))
  SHAR_REL_LIB = $(MY_TARGET:%=$(SHAR_REL_OBJDIR)/lib%.so$(SHAR_LIB_EXT))
  SHAR_DBG_LIB = $(MY_TARGET:%=$(SHAR_DBG_OBJDIR)/lib%$(DBG_SUFFIX).so$(SHAR_LIB_EXT))
  MODL_REL_LIB = $(MY_TARGET:%=$(MODL_REL_OBJDIR)/lib%_mod.$(STAT_LIB_EXT))
  MODL_DBG_LIB = $(MY_TARGET:%=$(MODL_DBG_OBJDIR)/lib%_mod$(DBG_SUFFIX).$(STAT_LIB_EXT))
endif

STAT_REL_OBJS = $(COMPILED_OBJS:%.$(STAT_OBJS_EXT)=$(STAT_REL_OBJDIR)/%.$(STAT_OBJS_EXT))
STAT_REL_DEPS = $(STAT_REL_OBJS:%.$(STAT_OBJS_EXT)=%.d)


STAT_DBG_OBJS = $(COMPILED_OBJS:%.$(STAT_OBJS_EXT)=$(STAT_DBG_OBJDIR)/%.$(STAT_OBJS_EXT))
STAT_DBG_DEPS = $(STAT_DBG_OBJS:%.$(STAT_OBJS_EXT)=%.d)


SHAR_REL_OBJS = $(COMPILED_OBJS:%.$(STAT_OBJS_EXT)=$(SHAR_REL_OBJDIR)/%.$(STAT_OBJS_EXT))
SHAR_REL_DEPS = $(SHAR_REL_OBJS:%.$(STAT_OBJS_EXT)=%.d)


SHAR_DBG_OBJS = $(COMPILED_OBJS:%.$(STAT_OBJS_EXT)=$(SHAR_DBG_OBJDIR)/%.$(STAT_OBJS_EXT))
SHAR_DBG_DEPS = $(SHAR_DBG_OBJS:%.$(STAT_OBJS_EXT)=%.d)

MODL_REL_OBJS = $(COMPILED_OBJS:%.$(STAT_OBJS_EXT)=$(MODL_REL_OBJDIR)/%.$(STAT_OBJS_EXT))
MODL_REL_DEPS = $(MODL_REL_OBJS:%.$(STAT_OBJS_EXT)=%.d)

MODL_DBG_OBJS = $(COMPILED_OBJS:%.$(STAT_OBJS_EXT)=$(MODL_DBG_OBJDIR)/%.$(STAT_OBJS_EXT))
MODL_DBG_DEPS = $(MODL_DBG_OBJS:%.$(STAT_OBJS_EXT)=%.d)

ifeq ($(LIBTARGET),static-dbg)
  override LIBTARGET = $(STAT_DBG_LIB)
endif

ifeq ($(LIBTARGET),static-rel)
  override LIBTARGET = $(STAT_REL_LIB)
endif

ifeq ($(LIBTARGET),shared-dbg)
  override LIBTARGET = $(SHAR_DBG_LIB)
endif

ifeq ($(LIBTARGET),shared-rel)
  override LIBTARGET = $(SHAR_REL_LIB)
endif

ifeq ($(LIBTARGET),module-dbg)
  override LIBTARGET = $(MODL_DBG_LIB)
endif

ifeq ($(LIBTARGET),module-rel)
  override LIBTARGET = $(MODL_REL_LIB)
endif

ifeq ($(LIBTARGET),$(STAT_REL_LIB))
  COMPILED_TARGETS = $(STAT_REL_LIB)
  DEPS = $(STAT_REL_DEPS)
  TARGETS_TO_INSTALL+=static
  CLEAN+=$(STAT_REL_OBJS) $(STAT_REL_DEPS)

  library-install: static-install

.PHONY:: library-install
endif


ifeq ($(LIBTARGET),$(STAT_DBG_LIB))
  COMPILED_TARGETS = $(STAT_DBG_LIB)
  DEPS = $(STAT_DBG_DEPS)
  TARGETS_TO_INSTALL+=static
  CLEAN+=$(STAT_DBG_OBJS) $(STAT_DBG_DEPS)

  library-install: static-install

.PHONY:: library-install
endif


ifeq ($(LIBTARGET),$(SHAR_REL_LIB))
  COMPILED_TARGETS = $(SHAR_REL_LIB)
  DEPS = $(SHAR_REL_DEPS)
  XCFLAGS += $(SHARED_CFLAGS)
  XCXXFLAGS += $(SHARED_CXXFLAGS)
  XLDFLAGS += $(SHARED_LDFLAGS)
  LIBNAME_NO_VERSION = $(MY_TARGET:%=lib%.so)
  TARGETS_TO_INSTALL+=shared
  ifeq ($(strip $(SONAME)),)
    SONAME = $(MY_TARGET:%=lib%.so$(SONAME_EXT))
  endif
  CLEAN+=$(SHAR_REL_OBJS) $(SHAR_REL_DEPS)

  library-install: shared-install

.PHONY:: library-install
endif

ifeq ($(LIBTARGET),$(SHAR_DBG_LIB))
  COMPILED_TARGETS = $(SHAR_DBG_LIB)
  DEPS = $(SHAR_DBG_DEPS)
  XCFLAGS += $(SHARED_CFLAGS)
  XCXXFLAGS += $(SHARED_CXXFLAGS)
  XLDFLAGS += $(SHARED_LDFLAGS)
  LIBNAME_NO_VERSION = $(MY_TARGET:%=lib%$(DBG_SUFFIX).so)
  TARGETS_TO_INSTALL+=shared
  ifeq ($(strip $(SONAME)),)
    SONAME = $(MY_TARGET:%=lib%$(DBG_SUFFIX).so$(SONAME_EXT))
  endif
  CLEAN+=$(SHAR_DBG_OBJS) $(SHAR_DBG_DEPS)

  library-install: shared-install

.PHONY:: library-install
endif

ifeq ($(LIBTARGET),$(MODL_REL_LIB))
  COMPILED_TARGETS = $(MODL_REL_LIB)
  DEPS = $(MODL_REL_DEPS)
  XCFLAGS += $(SHARED_CFLAGS)
  XCXXFLAGS += $(SHARED_CXXFLAGS)
  XLDFLAGS += $(SHARED_LDFLAGS)
  CLEAN+=$(MODL_REL_OBJS) $(MODL_REL_DEPS)

  library-install: module-install

.PHONY:: library-install
endif

ifeq ($(LIBTARGET),$(MODL_DBG_LIB))
  COMPILED_TARGETS = $(MODL_DBG_LIB)
  DEPS = $(MODL_DBG_DEPS)
  XCFLAGS += $(SHARED_CFLAGS)
  XCXXFLAGS += $(SHARED_CXXFLAGS)
  XLDFLAGS += $(SHARED_LDFLAGS)
  CLEAN+=$(MODL_DBG_OBJS) $(MODL_DBG_DEPS)

  library-install: module-install

.PHONY:: library-install
endif


REALTARGET = $(LIBTARGET)




static_srcdir=$(patsubst %/,%,$(dir $(LIBTARGET)))
static_destdir=$(DESTDIR)
static_files=$(notdir $(LIBTARGET))
static_ifunction=_static_install_

ifneq ($(RANLIB),)
  RANDOMLIB=$(RANLIB) $(static_destdir)/$(static_files)
endif


define _static_install_
	$(INSTALL) -c -m 664 WHAT_TO_INSTALL $(static_destdir)
	$(RANDOMLIB)
endef

shared_srcdir=$(patsubst %/,%,$(dir $(LIBTARGET)))
shared_destdir=$(DESTDIR)
shared_files=$(notdir $(LIBTARGET))
shared_ifunction=_shared_install_

define _shared_install_
	$(INSTALL) -c -m 775 WHAT_TO_INSTALL $(shared_destdir)
        @if [ $(notdir $(LIBTARGET)) != $(LIBNAME_NO_VERSION) ] ; then cd $(shared_destdir) ; rm -f $(LIBNAME_NO_VERSION) ; ln -sf $(notdir $(LIBTARGET)) $(LIBNAME_NO_VERSION); fi
    
endef

module_srcdir=$(patsubst %/,%,$(dir $(LIBTARGET)))
module_destdir=$(DESTDIR)
module_files=$(notdir $(LIBTARGET))
module_ifunction=_module_install_

define _module_install_
	$(INSTALL) -c -m 775 WHAT_TO_INSTALL $(module_destdir)
endef

include $(MK)/rules.mk


define build_shared
	-$(RM) $@
	$(SHARED_LINK) $(SONAME_ARG) -o $@  $(filter-out -l%,$(LDFLAGS)) \
		$^ $(filter -l%,$(LDFLAGS)) $(POST_LDFLAGS)
	-chmod g+w $@
endef



# If $(BUILD_LIBRARY) isn't defined, use GNUmake's special archive
# dependencies to build individual members as needed.  Otherwise,
# make the finished library depend explicitly on the objects and
# use $(BUILD_LIBRARY) to build it.


static-rel: 
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(STAT_REL_LIB) RELEASE=1

static-dbg: 
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(STAT_DBG_LIB) 

shared-dbg: 
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(SHAR_DBG_LIB) 

shared-rel: 
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(SHAR_REL_LIB) RELEASE=1

module-dbg: 
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(MODL_DBG_LIB) 

module-rel: 
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(MODL_REL_LIB) RELEASE=1

install-static-rel: 
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(STAT_REL_LIB) RELEASE=1 install

install-static-dbg: 
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(STAT_DBG_LIB) install

install-shared-dbg: 
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(SHAR_DBG_LIB) install

install-shared-rel: 
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(SHAR_REL_LIB) RELEASE=1 install

install-module-dbg: 
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(MODL_DBG_LIB) install

install-module-rel: 
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(MODL_REL_LIB) RELEASE=1 install


all-libs:
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(STAT_REL_LIB)
	$(MAKE) LIBTARGET=$(STAT_DBG_LIB)
	$(MAKE) LIBTARGET=$(SHAR_REL_LIB)
	$(MAKE) LIBTARGET=$(SHAR_DBG_LIB)
	$(MAKE) LIBTARGET=$(MODL_REL_LIB)
	$(MAKE) LIBTARGET=$(MODL_DBG_LIB)

install-all:
	@export MAKEFLAGS
	$(MAKE) LIBTARGET=$(STAT_REL_LIB) install
	$(MAKE) LIBTARGET=$(STAT_DBG_LIB) install
	$(MAKE) LIBTARGET=$(SHAR_REL_LIB) install
	$(MAKE) LIBTARGET=$(SHAR_DBG_LIB) install
	$(MAKE) LIBTARGET=$(MODL_REL_LIB) install
	$(MAKE) LIBTARGET=$(MODL_DBG_LIB) install

ifeq ($(BUILD_LIBRARY),)

$(STAT_REL_LIB): $(STAT_REL_OBJS:%=$(STAT_REL_LIB)(%))

$(STAT_DBG_LIB): $(STAT_DBG_OBJS:%=$(STAT_DBG_LIB)(%))

print-lobjs:
	@echo $(SHAR_DBG_OBJS)	
	@echo $(STAT_DBG_OBJS)	

.PHONY:: print-lobjs

else

.PRECIOUS:: $(STAT_REL_OBJS) $(STAT_DBG_OBJS)

$(STAT_REL_LIB):: $(STAT_REL_OBJS)
	-$(RM) $@
	$(BUILD_LIBRARY)

$(STAT_DBG_LIB):: $(STAT_DBG_OBJS)
	-$(RM) $@
	$(BUILD_LIBRARY)

print-lobjs:
	@echo $(STAT_DBG_OBJS)	

.PHONY:: print-lobjs

endif

$(SHAR_REL_LIB):: $(SHAR_REL_OBJS)
	$(build_shared)

$(SHAR_DBG_LIB):: $(SHAR_DBG_OBJS)
	$(build_shared)


$(MODL_REL_LIB): $(MODL_REL_OBJS:%=$(MODL_REL_LIB)(%))

$(MODL_DBG_LIB): $(MODL_DBG_OBJS:%=$(MODL_DBG_LIB)(%))

# Rules for checking in files to the configuration 
# management/source control system
ifeq ($(strip $(CM_DESTDIR)),)
 TMP_CM_DIRS = $(shell $(FIND_DIR) 0 sdk_lib $(TOP))
 TMP_CM_DIRS += $(shell $(FIND_DIR) 0 SDK_lib $(TOP))
CM_DESTDIR = $(firstword $(TMP_CM_DIRS))/$(BUILD_ARCH)
endif


ifeq ($(strip $(CM_DESTDIR)),)
cm-release:
	@echo "ERROR -- Must define CM_DESTDIR to location where target should be checked-in"
	/bin/false;

.PHONY:: cm-release
else

ifeq ($(strip $(CM_LABEL)),)
cm-release:
	@error "Must define CM_LABEL to the label name that will be applied to the target"
	/bin/false

.PHONY:: cm-release

else 

cm-release: $(LIBTARGET)
	#@echo "Running cm-release on target $(LIBTARGET), dest = $(CM_DESTDIR)/$(BUILD_ARCH), label = $(CM_LABEL) "
	$(MK)/../bin/newcob.pl --label $(CM_LABEL) $(CM_BRANCH) $(REALTARGET) $(CM_DESTDIR)

.PHONY:: cm-release
endif #CM_LABEL is NULL

endif #CM_DESTDIR is NULL



endif
endif
