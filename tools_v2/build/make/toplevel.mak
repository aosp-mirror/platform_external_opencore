#
# This makefile should be included by makefiles at the project level.
# See also recursive.mk
#

.PHONY: world

world: install_hdrs install_libs install_bins

include $(MK)/recursive.mk
