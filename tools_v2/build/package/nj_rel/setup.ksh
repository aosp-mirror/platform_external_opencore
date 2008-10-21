echo Setting up build environment with default configuration ...

# SDK_LOCAL is the local staging area (i.e., view private) for private
# builds and installs of the corelibs libraries.  The path should be 
# relative to VOB_BASE_DIR.  Set to a default value if it is not set 
if [[ -z $SDK_LOCAL ]]; then
  export SDK_LOCAL=$PWD
  echo "Set SDK_LOCAL to $SDK_LOCAL ..."
else
  echo "SDK_LOCAL already defined as $SDK_LOCAL"
fi

# Set CFG_DIR
#
export CFG_DIR=$PWD
echo "Set CFG_DIR to $CFG_DIR ..."

# For picking the node registry and tunables, set this flag
#
export FORMAT=nj

export ANDROID_BASE=/opt/environments/nj/android
echo Set ANDROID_BASE to $ANDROID_BASE

export KERNEL_HEADERS=$ANDROID_BASE/device/system/kernel_headers
echo Set KERNEL_HEADERS to $KERNEL_HEADERS

# Setup the default environment
#
. ../default/setup.ksh
