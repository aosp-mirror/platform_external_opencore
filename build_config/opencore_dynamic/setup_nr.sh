echo Setting up build environment with default configuration ...

export PROJECT_DIR=$PWD

# Set CFG_DIR
#
export CFG_DIR=$PWD
echo "Set CFG_DIR to $CFG_DIR ..."

# Setup the default environment
#
. ../default/setup_nr.sh

# include the android definitions for export to android makefiles.
export PLATFORM_EXTRAS=$MK/android.mk

# For make completion targets
mkcmdcmpl




