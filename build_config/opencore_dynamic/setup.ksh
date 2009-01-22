echo Setting up build environment with default configuration ...

# SDK_LOCAL is the local staging area (i.e., view private) for private
# builds and installs of the corelibs libraries.  
export SDK_LOCAL=$PWD
echo "Set SDK_LOCAL to $SDK_LOCAL ..."


# Set CFG_DIR
#
export CFG_DIR=$PWD
echo "Set CFG_DIR to $CFG_DIR ..."

# For picking the node registry and tunables, set this flag
#
export FORMAT=nj

# Setup the default environment
#
if [[ -z $DEFAULT_SETUP_PATH ]]; then
  export DEFAULT_SETUP_PATH=../default
fi

. $DEFAULT_SETUP_PATH/setup.ksh
