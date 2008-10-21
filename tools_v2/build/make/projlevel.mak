include project.mk

BUILDDIRS = $(BUILDPATH:%=$(TOP)/%)

project:
	recursive_make "$(MAKE)" "$(MKFLAGS)" install_hdrs $(BUILDDIRS)
	recursive_make "$(MAKE)" "$(MKFLAGS)" install_libs $(BUILDDIRS)
	recursive_make "$(MAKE)" "$(MKFLAGS)" install_bins $(BUILDDIRS)

include $(MK)/recursive.mk
