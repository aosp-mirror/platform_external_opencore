ifndef EXTERNAL_OPENCORE_CONFIG_ONCE
  # This is the first attempt to include this file.
  EXTERNAL_OPENCORE_CONFIG_ONCE := true

  PV_TOP := $(my-dir)
  PV_CFLAGS := -Wno-non-virtual-dtor -DENABLE_SHAREDFD_PLAYBACK
  FORMAT := nj

  # HAS_OSCL_LIB_SUPPORT turns on PV's OSCL dynamic loader.
  # Set PV_OSCL_LIB to true to enable it (default in release mode).
  # To enable the value added modules, HAS_OSCL_LIB_SUPPORT must be true.
  #
  # However for debugging with gdb you may want to set PV_OSCL_LIB to
  # false to disable this custom loader.
  PV_OSCL_LIB := true
  ifeq ($(PV_OSCL_LIB), true)
    PV_CFLAGS += -DHAS_OSCL_LIB_SUPPORT
  endif

  # To enable the windows codecs under vendor/pv, define VALUE_ADD to true.
  # You need HAS_OSCL_LIB_SUPPORT as well for VALUE_ADD modules.
  VALUE_ADD := true
  ifeq ($(VALUE_ADD), true)
    PV_CFLAGS += -DPV_USE_VALUE_ADD=1
  endif

  PV_COPY_HEADERS_TO := libpv

  PV_INCLUDES := \
	$(PV_TOP)/android \
	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
	$(PV_TOP)/engines/common/include \
	$(PV_TOP)/engines/player/config/linux_nj \
	$(PV_TOP)/engines/player/include \
	$(PV_TOP)/nodes/pvmediaoutputnode/include \
	$(PV_TOP)/nodes/pvdownloadmanagernode/config/opencore \
	$(PV_TOP)/pvmi/pvmf/include \
	$(PV_TOP)/fileformats/mp4/parser/config/opencore \
	$(PV_TOP)/oscl/oscl/config/linux_nj \
	$(PV_TOP)/oscl/oscl/config/shared \
	$(PV_TOP)/engines/author/include \
	$(PV_TOP)/android/drm/oma1/src \
	$(TARGET_OUT_HEADERS)/$(PV_COPY_HEADERS_TO) 

  # Stash these values for the next includer of this file.
  OPENCORE.PV_TOP := $(PV_TOP)
  OPENCORE.PV_CFLAGS := $(PV_CFLAGS)
  OPENCORE.FORMAT := $(FORMAT)
  OPENCORE.PV_OSCL_LIB := $(OPENCORE.PV_OSCL_LIB)
  OPENCORE.PV_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
  OPENCORE.VALUE_ADD := $(VALUE_ADD)
  OPENCORE.PV_INCLUDES := $(PV_INCLUDES)
else
  # This file has already been included by someone, so we can
  # use the precomputed values.
  PV_TOP := $(OPENCORE.PV_TOP)
  PV_CFLAGS := $(OPENCORE.PV_CFLAGS)
  FORMAT := $(OPENCORE.FORMAT)
  PV_OSCL_LIB := $(OPENCORE.PV_OSCL_LIB)
  PV_COPY_HEADERS_TO := $(OPENCORE.PV_COPY_HEADERS_TO)
  VALUE_ADD := $(OPENCORE.VALUE_ADD)
  PV_INCLUDES := $(OPENCORE.PV_INCLUDES)
endif
