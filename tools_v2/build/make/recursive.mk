
SUBDIRS = $(LIBDIRS) $(BINDIRS) $(TESTDIRS) $(LIBDIRS-m)

RECURSIVE_MAKE = $(MK)/../bin/recursive_make

ifneq ($(SUBDIRS),)

# The NOHDRINST macro is used to bypass the headers-install
# build step.  It is useful for situations where all the header
# files are already visible within the compiler search path.
# ONLY DEFINE THE MACRO IF YOU KNOW WHAT YOU'RE DOING
ifdef NOHDRINST
  world: library-install install-it
else
  world: headers-install library-install install-it
endif

ifeq ($(HOST_ARCH), win32)
  headers-install:
	@export MAKEFLAGS
	echo "Recursive"
	$(RECURSIVE_MAKE) "\"$(MAKE) "NODEPS=1"\"" headers-install "$(LIBDIRS)" "$(LIBDIRS-m)"

  library-install:
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE)" library-install "$(LIBDIRS)"

  install-it:
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE)" install "$(BINDIRS)"

  clean::
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE)"  $@ "$(SUBDIRS)"

  cm-release::
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE)"  cm-release "$(LIBDIRS)" "$(BINDIRS)"

  run_test::
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE)"  run_test "$(TESTDIRS)"


  %::
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE)"  "$@" "$(SUBDIRS)"
else
  headers-install:
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE) NODEPS=1" headers-install $(LIBDIRS) $(LIBDIRS-m)

  library-install:
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE)" library-install $(LIBDIRS)
	$(RECURSIVE_MAKE) "$(MAKE)" "" $(LIBDIRS-m)

  install-it:
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE)" install $(BINDIRS)

  clean::
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE)"  $@ $(SUBDIRS)

  cm-release::
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE)"  cm-release $(LIBDIRS) $(BINDIRS)

  run_test::
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE)"  run_test $(TESTDIRS)


  %::
	@export MAKEFLAGS
	$(RECURSIVE_MAKE) "$(MAKE)"  $@ $(SUBDIRS)

endif
  .PHONY:: world install-it headers-install library-install run_test cm-release

else

  .DEFAULT:
	@echo "No subdirectories defined."

endif
