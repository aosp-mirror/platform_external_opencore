#
# This customize makefile template should contain the
# Macros, Include Paths, SRCS, HDRS, LIBTARGET-m, LIBTARGET-y
# values specific to nj package
#

export FORMAT=nj

LIBDIRS-m += $(VOB_BASE_DIR)/tools_v2/build/package/nj_rel/module/pvcommon \
             $(VOB_BASE_DIR)/tools_v2/build/package/nj_rel/module/pvplayer \
	     $(VOB_BASE_DIR)/tools_v2/build/package/nj_rel/module/pvauthor
