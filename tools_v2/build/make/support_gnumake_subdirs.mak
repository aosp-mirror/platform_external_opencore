#
#BEGINDOC
# -----------------------------------------------------------------------------
#
# This GNUmake template builds targets in subdirectories.  By default, the
# "install" target will be built if none are specified on the command line.
#
# SUPPORTED MACROS:
#
# SRCS		A list of the subdirectories to build.  If not specified,
#		all subdirectories containing a file called "GNUmakefile"
#		will be built.
#
# 
# -----------------------------------------------------------------------------
#ENDOC

include $(MK)/local.mk

ifeq ($(SRCS),)
	GNUMAKEFILES=$(wildcard */GNUmakefile)
	DIRLIST=$(subst /GNUmakefile,,$(GNUMAKEFILES))
else
	DIRLIST=$(SRCS)
endif

.PHONY:: all
all:: install

%: $(addsuffix -%-subdir,$(DIRLIST))
	@echo -n ""

ifeq ($(BCS)$(IN_STAGING),11)
    define _stage_subdir_
      -stage $(firstword $(subst -, , $(subst -subdir,,$@)))
    endef
endif

%-subdir:
	$(_stage_subdir_)
	$(MAKE) $(MFLAGS) -C $(subst -, , $(subst -subdir,,$@))



