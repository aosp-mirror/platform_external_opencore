# This file is at the heart of this makefile system.  It defines the
# main rules for building software.
#
# There are many variables used in this file.  It is appropriate to
# modify or override some in unit-level makefiles or in project.mk
# and local.mk.  Others should not be tampered with.  When possible,
# variables are set up here to support gnu make's default targets as
# documented in the gnu make manual.
#
# ARCHITECTURE-DEPENDENT VARIABLES
#   These variables are suitable for modification in the arch.mk files:
#
#     CC:   The C compiler
#     CXX:  The C++ compiler
#     CO:   The compiler option used to specify object file name
#     LD:   The linker
#     RM:   Remove a file -- should succeed if the file does not exist
#     AR:   Library archive builder
#     AR_FLAGS:  Flags to pass to ar when adding an object file to a
#		 library
#     RANLIB: Program used to generate library tables of contents.  (On
#	      systems with no ranlib program, this can be defined as
#	      "true" since the "true" program always succeeds.)
#
# VARIABLES SUITABLE FOR LOCAL MODIFICATION
#   These variables can be modified in individual makefiles,
#   in local.mk, or in project.mk.  They should be set += so
#   that settings are not overridden if set in more than once
#   place:
#
#     DFLAGS: Debugging flags to the compiler.  Flags defined here
#         are used during compiling and linking but not during
#         dependency generation.
#     XLIBDIRS: Extra directories (outside project library
#         directories and default directories) where libraries
#	  should be searched for.
#     ENVCPPFLAGS:  Global flags that should be set on the command
#         line.  This flags must not conflict with the XCPPFLAGS
#         values inside the individual makefiles.  
#     XCPPFLAGS: Extra flags for the C Preprocessor besides
#         the standard and deafult include directories.  This
#	  could include things like -DDEBUG.
#     XCFLAGS: Extra flags to the C compiler that are not used
#         for debugging or the preprocessor.  These will
#	  generally be compiler-specific.  (With gcc, for
#	  example, turning on -Wall here would be appropriate.)
#     XCXXFLAGS: Same as XCFLAGS but for the C++ compiler.
#     XLDFLAGS: Sames as XCFLAGS but used at link time.
#     
# GUIDELINES FOR MODIFYING THIS FILE
#   If there is not an appropriate place in one of the above
#   variables for something you need to include, rather than
#   modifying one of the others, add the additional variable to
#   the list and included where necessary.  For example, If you
#   wanted to add additional flags to be used only during
#   dependency generation, don't hard code them into the
#   dependency generation rules.  Instead, add something like
#   DEPFLAGS and include that in the rule.  In general, users
#   should be able to treat this file like a black box and
#   should not need to be concerned with the specifics of how it
#   does its job.
#

ifeq  ($(strip $(MAKE)),clearmake)
CCASE_VIEW_TYPE := $(shell $(MK)/../bin/ccase_view_type)
endif

# if BUILD_ARCH not set then set to unknown
ifeq ($(strip $(BUILD_ARCH)),)
  BUILD_ARCH = unknown
endif

# if OBJDIRS not set then set to BUILD_ARCH
ifeq ($(strip $(OBJDIRS)),)
  OBJDIRS = $(BUILD_ARCH)
endif

ifeq ($(strip $(COMPILED_TARGETS)),)
  COMPILED_TARGETS = $(REALTARGET)
endif



include $(MK)/global.mk

CFLAGS = $(DFLAGS) $(XCFLAGS) $(COMPFLAGS)
CXXFLAGS = $(DFLAGS) $(XCXXFLAGS) $(COMPFLAGS)
CPPFLAGS +=  $(ENVCPPFLAGS) $(XCPPFLAGS) $(INCDIRS) $(XINCDIRS) $(POST_INCDIRS) 

# if REALLIBS IS NOT SET THEN USE LIBS
# ELSE USE REALLIBS AS MODIFIED USER LIST 
# OF LIBS
ifeq ($(strip $(REALLIBS)),)
  ifneq ($(strip $(RELEASE)),)
    # compiling in RELEASE mode so check for DEBUG_LIBS macro
    ALL_PV_LIBS = $(foreach lib, $(LIBS), $(patsubst $(filter $(lib), $(DEBUG_LIBS)), $(lib)$(DBG_SUFFIX), $(lib))) 
  else 
    #compiling in debug mode so map all libs by default to _debug
    TMP_DBG_LIBS = $(filter-out $(RELEASE_LIBS), $(LIBS))
    ALL_PV_LIBS = $(foreach lib, $(LIBS), $(patsubst $(filter $(lib), $(TMP_DBG_LIBS)), $(lib)$(DBG_SUFFIX), $(lib))) 
  endif
else 
  ifneq ($(strip $(RELEASE)),)
    # compiling in RELEASE mode so check for DEBUG_LIBS macro
    ALL_PV_LIBS = $(foreach lib, $(REALLIBS), $(patsubst $(filter $(lib), $(DEBUG_LIBS)), $(lib)$(DBG_SUFFIX), $(lib))) 
  else 
    #compiling in debug mode so map all libs by default to _debug
    TMP_DBG_LIBS = $(filter-out $(RELEASE_LIBS), $(REALLIBS))
    ALL_PV_LIBS = $(foreach lib, $(REALLIBS), $(patsubst $(filter $(lib), $(TMP_DBG_LIBS)), $(lib)$(DBG_SUFFIX), $(lib)))
  endif
endif

LDFLAGS += $(LIB_DIRS) $(ALL_PV_LIBS) $(SYSLIBS) $(DEFAULT_LD_FLAGS) $(XLDFLAGS)

# The variable ALL_LIBS will be used for the dependency list, but clearmake 
# has a bug where it doesn't handle the dependency if there is only a *.so 
# file and no *.a file.  We'll remove these from the dependency list for now.
ifeq  ($(strip $(MAKE)),clearmake)
  ALL_LIBS = $(ALL_PV_LIBS)
else
  ALL_LIBS = $(ALL_PV_LIBS) $(SYSLIBS)
endif

ifeq ($(strip $(LIBCOMPFLAG)),)
  LIBCOMPFLAG = -L
  LIB_DIRS += $(XLIBDIRS) $(POST_LIBDIRS)
else
  LIB_DIRS += $(XLIBDIRS:-L%=$(LIBCOMPFLAG)%) $(POST_LIBDIRS:-L%=$(LIBCOMPFLAG)%)
endif

print_deps: 
	echo $(DEPS)

print_libdirs:
	echo $(LIB_DIRS)

print_libs: 
	@echo LIBS = $(ALL_LIBS)

print_pv_libs:
	@echo LIBS = $(ALL_PV_LIBS)

print_make_val:
	echo $(MAKE) $(CCASE_VIEW_TYPE)


define create_objdir
	@$(MK)/../bin/cc_mkdir $(@D)
endef

ifneq ($(HOST_ARCH), win32)
define group_writable
	chmod g+w $(@D)
endef
endif

# The MS Visual C++ creates these files and clearmake stores them as sibling derived
# objects with an automatic dependency.  This causes unnecessary rebuilding.  This
# special target rule will prevent storing them as derived objects
ifeq ($(TOOLSET), cl)
.NO_DO_FOR_SIBLING : vc60.idb %.pdb %.ilk
endif

#
# Build dependency files
#

#
# This takes out all references to absolute files in the source tree
# and replaces them with references to $(TOP).  This eliminates
# the need to rebuild the .d's whenever we change from the baseline to
# a staging area or vice versa.  We change all forward slashes to backquotes
# so we don't give sed(1) fits.
#
define _process_dot_d_
	@mv $@ $@.tmp
	@$(SED) -e 's/\//`/g' $@.tmp \
	| $(SED) -e 's/$(subst /,`,$(TOP))/\$$\(TOP\)/g' \
	| $(SED) -e 's/`/\//g' \
	> $@
	@$(RM) $@.tmp
endef


ifeq ($(strip $(CDEPS)),)
DEPS_ARG=$(CC) -MM $(CPPFLAGS) $(CXXFLAGS)
else 
DEPS_ARG=$(CDEPS) $(CPPFLAGS) $(CXXFLAGS)
endif

DEPPAT = $(foreach dir, $(OBJDIRS), $(patsubst %, %/%.d, $(dir)))
OBJPAT = $(foreach dir, $(OBJDIRS), $(patsubst %, %/%.$(STAT_OBJS_EXT), $(dir)))

showgendeps:
	@echo gen_deps $(GEN_DEPS) 
	@echo cxx $(CXX) -M 
	@echo cppflags $(CPPFLAGS) 
	@echo realtarget $(REALTARGET) 
	@echo buildarch objdir keepobj $(BUILD_ARCH) $(OBJDIRS) $(KEEP_OBJ)

ifneq  ($(strip $(MAKE)),clearmake)
ifneq ($(REMOTE_SRCS),)
include remote.mk
endif
endif

$(DEPPAT) : %.c
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)

$(DEPPAT) : %.C
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)

$(DEPPAT) : %.cc
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)

$(DEPPAT) : %.cpp
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)

$(DEPPAT) : %.asm
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)

$(DEPPAT) : %.s
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)



$(DEPPAT) : $(SRCDIR)/%.c
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)

$(DEPPAT) : $(SRCDIR)/%.C
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)

$(DEPPAT) : $(SRCDIR)/%.cc
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)

$(DEPPAT) : $(SRCDIR)/%.cpp
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)


$(DEPPAT) : $(SRCDIR)/%.asm
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)

$(DEPPAT) : $(SRCDIR)/%.s
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)


%.d : %.asm
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)

%.d : %.s
	$(create_objdir)
	$(GEN_DEPS) "$(DEPS_ARG)" $(filter $(@D)/%, $(COMPILED_TARGETS)) $< $(@D) $(KEEP_OBJ)
	$(_process_dot_d_)
	$(_custom_process_dot_d_)


ifeq  ($(strip $(MAKE)),clearmake)
ifeq  ($(strip $(CCASE_VIEW_TYPE)),dynamic)
override NODEPS := 1
endif
endif



#
# Build objects
#

ifdef NODEPS
$(REALTARGET)(%.$(STAT_OBJS_EXT)) : %.$(STAT_OBJS_EXT)
	$(create_objdir)
	$(AR) $(AR_ARGS) $@ $*.$(STAT_OBJS_EXT)
	#$(RM) $<
else
$(REALTARGET)(%.$(STAT_OBJS_EXT)) : %.$(STAT_OBJS_EXT) %.d
	$(create_objdir)
	$(AR) $(AR_ARGS) $@ $*.$(STAT_OBJS_EXT)
	#$(RM) $<
endif


%.obj : %.c %.d
	$(create_objdir)
	-$(RM) $@
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CO)$@ $<
	$(group_writable)

%.obj : %.asm
	$(create_objdir)
	-$(RM) $@
	$(ASM) $(ASMFLAGS) $(CO)$@ $<
	$(group_writable)

%.o : %.s
	$(create_objdir)
	-$(RM) $@
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : %.c
	$(create_objdir)
	-$(RM) $@
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : %.C
	$(create_objdir)
	-$(RM) $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : %.cc
	$(create_objdir)
	-$(RM) $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : %.cpp
	$(create_objdir)
	-$(RM) $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : %.s
	$(create_objdir)
	-$(RM) $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CO)$@ $<
	$(group_writable)

# object patterns in family directory
$(OBJPAT) : $(FAMILY_DIR)/%.c
	$(create_objdir)
	-$(RM) $@
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : $(FAMILY_DIR)/%.C
	$(create_objdir)
	-$(RM) $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : $(FAMILY_DIR)/%.cc
	$(create_objdir)
	-$(RM) $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : $(FAMILY_DIR)/%.cpp
	$(create_objdir)
	-$(RM) $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : $(FAMILY_DIR)/%.s
	$(create_objdir)
	-$(RM) $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CO)$@ $<
	$(group_writable)

# object patterns in family directory
$(OBJPAT) : $(SRCDIR)/%.c
	$(create_objdir)
	-$(RM) $@
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : $(SRCDIR)/%.C
	$(create_objdir)
	-$(RM) $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : $(SRCDIR)/%.cc
	$(create_objdir)
	-$(RM) $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : $(SRCDIR)/%.cpp
	$(create_objdir)
	-$(RM) $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CO)$@ $<
	$(group_writable)

$(OBJPAT) : $(SRCDIR)/%.s
	$(create_objdir)
	-$(RM) $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CO)$@ $<
	$(group_writable)


.PHONY:: all clean print-objpat


all:: $(REALTARGET)

print-objpat: 
	@echo $(OBJPAT) $(FAMILY_DIR) $(SRCDIR)

ETAGS_OPTS = -a
CTAGS_OPTS = -a -t -T -w -S
TAG_HDRS := $(wildcard *.h)
ifeq ($(TOOLSET),g++)
TAG_HDR_OPTS = -C
endif
ifeq ($(TOOLSET),cxx)
TAG_HDR_OPTS = -C
endif
ifneq ($(strip $(TAG_HDRS)),)
define _build_hdr_etags
	etags $(ETAGS_OPTS) $(TAG_HDR_OPTS) $(TAG_HDRS)
endef
define _build_hdr_ctags
	ctags $(CTAGS_OPTS) $(TAG_HDR_OPTS) $(TAG_HDRS)
endef
endif


############################################
# Create tags for emacs, stored in file $(ETAG_FILE) 
#    which is set in enviroment.
#    e.g., ETAG_FILE= /auto_home/miao/pvs_etags
#  and we do not create tags for test files.
ETAGS_OPTS += --c++ --members
ETAGS_SRC_FILES = $(SRCS)  $(HDRS)
tmpp = $(notdir $(shell $(MK)/../bin/cc_pwd))
ifneq ($(notdir $(shell $(MK)/../bin/cc_pwd)),test)
ifneq ( $(ETAGS_SRC_FILES),)
define _create_etags
	etags $(ETAGS_SRC_FILES)  $(ETAGS_OPTS) -o $(ETAG_FILE)
endef
else
define _create_etags
	@echo "No files for tags in this dir."
endef
endif

else
define _create_etags
	@echo "Skipping test dir."
endef
endif

etags::
	$(_create_etags)

newetags::
	-$(RM) $(ETAG_FILE)
	$(_create_etags)

TAGS::
	-$(RM) $@
	etags $(ETAGS_OPTS) $(CSRCS) $(CCSRCS) $(CXXSRCS)
	-$(_build_hdr_etags)

tags::
	-$(RM) $@
	ctags $(CTAGS_OPTS) $(CSRCS) $(CCSRCS) $(CXXSRCS)
	-$(_build_hdr_ctags)


# The assignments below attempt to sort out which of the dependency
# files are no longer valid because of name changes in header files.
#
# Explanation of variables:
#
# DEPS_EXISTING	= Dependency files that already exist
# DEPS_MISSING	= Dependency files that don't already exist
# DEPS_HEADERS	= List of the header files mentioned in $(DEPS_EXISTING)
# DEPS_HFOUND	= 
# DEPS_HMISSING	= List of headers from $(DEPS_HEADERS) that don't actually
#		  exist.
# DEPS_FBAD	= A list of the dependency files that were bad
#
# Note that we only go through this rigamarole if NODEPS hasn't been
# defined.  All assignments are of the simply-expanded variety so we
# don't do any excessive calls to the shell.


ifndef NODEPS

    DEPS_EXISTING := $(wildcard $(DEPS))
    DEPS_MISSING  := $(filter-out $(DEPS_EXISTING),$(DEPS))

    # If we found any dependency files at all, check to see if any header
    # files are mentioned that don't exist.

    ifneq ($(DEPS_EXISTING),)

        # Make a list of the headers mentioned in the dependency files.
        # We put the word $(TOP) in, so substitute the actual value of
        # $(TOP) whenever we see $$(TOP).

        DEPS_HEADERS := $(subst $$(TOP), $(TOP), \
            $(sort $(filter %.h,$(shell cat $(DEPS_EXISTING)))) )

        # Figure out which headers exist and which don't.

        DEPS_HFOUND := $(wildcard $(DEPS_HEADERS))
        DEPS_HMISSING := $(filter-out $(DEPS_HFOUND),$(DEPS_HEADERS))

        # If there were any headers missing, take down the names of
        # the dependency files the used them and wipe them out.

        ifneq ($(DEPS_HMISSING),)
            DEPS_FBAD := $(shell echo $(DEPS_HMISSING) > .makejunk ; \
                fgrep -l -f .makejunk $(DEPS) ; rm .makejunk )
            DEPS_JUNK := $(shell $(RM) $(DEPS_FBAD))
        endif

    endif

    # If any dependency files are missing, rebuild them before they're
    # included.

    ifneq ($(DEPS_MISSING)$(DEPS_FBAD),)
        DEPS_JUNK := $(shell $(MAKE) NODEPS=1 \
	COMPILED_TARGETS=$(COMPILED_TARGETS) KEEP_OBJ=$(KEEP_OBJ)\
            $(DEPS_MISSING) $(DEPS_FBAD) 2>&1)
    endif

    DEPS_TO_INCLUDE := $(wildcard $(DEPS))

    ifneq ($(DEPS_TO_INCLUDE),)
        include $(DEPS_TO_INCLUDE)
    endif
endif

.PRECIOUS:: $(DEPS)

include $(MK)/utilities.mk

