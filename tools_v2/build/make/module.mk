#
# This makefile template should be included by makefiles to
# build runtime loadable modules.
#

MY_TARGET = $(strip $(TARGET))

# if DBG_SUFFIX not set then set to _debug
ifeq ($(strip $(DBG_SUFFIX)),)
  DBG_SUFFIX = _debug
endif

LIBS_LINKED = $(foreach lib, $(LIBS), $(patsubst $(filter $(lib), $(LIBS)), $(lib)$(DBG_SUFFIX), $(lib)))
MODS_LINKED = $(foreach mod, $(MODS), $(patsubst $(filter $(mod), $(MODS)), $(mod)$(DBG_SUFFIX), $(mod)))

ifeq ($(RELEASE),1)
  override SOTARGET = $(MODL_REL_LIB)
  MODL_OBJDIR = $(BUILD_ARCH)/module_rel
  LIBS_LINKED = $(foreach lib, $(LIBS), $(lib)_mod)
  MODS_LINKED = $(foreach mod, $(MODS), $(mod))
  LIBTARGET   = module-rel
else
  override SOTARGET = $(MODL_DBG_LIB)
  MODL_OBJDIR = $(BUILD_ARCH)/module_dbg
  LIBS_LINKED = $(foreach lib, $(LIBS), $(lib)_mod$(DBG_SUFFIX))
  MODS_LINKED = $(foreach mod, $(MODS), $(mod)$(DBG_SUFFIX))
  LIBTARGET   = module-dbg
endif

entire: headers-install LIBTARGET=$(LIBTARGET) $(SOTARGET) so-install

CLEAN += $(SOTARGET)

MODL_REL_LIB = $(MY_TARGET:%=$(MODL_OBJDIR)/lib%.so$(SHAR_LIB_EXT))
MODL_DBG_LIB = $(MY_TARGET:%=$(MODL_OBJDIR)/lib%$(DBG_SUFFIX).so$(SHAR_LIB_EXT))

XLDFLAGS  += $(SHARED_LDFLAGS)

ifeq ($(strip $(SONAME)),)
    SONAME = $(SOTARGET)$(SONAME_EXT)
endif

so_srcdir=$(patsubst %/,%,$(dir $(SOTARGET)))
so_destdir=$(DESTDIR)
so_files=$(notdir $(SOTARGET))
so_ifunction=_so_install_

define _so_install_
	$(INSTALL) -c -m 775 WHAT_TO_INSTALL $(so_destdir)
endef

include $(MK)/rules.mk

include $(MK)/recursive.mk

$(SOTARGET):
	@$(MK)/../bin/cc_mkdir $(MODL_OBJDIR) 
	$(build_shared)

define build_shared
	-$(RM) $@
	$(SHARED_LINK) $(SONAME_ARG) $(XLDFLAGS) -o $@ $(LIBS_LINKED) $(MODS_LINKED) $(POST_LIBDIRS) $(POST_SHARED_LDFLAGS)
	-chmod g+w $@
endef
.PHONY:: entire
