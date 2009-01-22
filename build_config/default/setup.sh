echo Setting up build environment with default configuration ...

# Setup environment for necessary for the
# makefiles, etc.

# Set this flag for integarting CML2 config with the builds.
# 
export USE_CML2_CONFIG=1

# How is VOB_BASE_DIR value set?
# 1. If there is an argument to the setup script execution, use that. 
# 2. Else, check if the value is already set and use that. 
# 3. Throw an error.
# @TODO : Obsolete VOB_BASE_DIR when that macro is no longer used.

if [[ $# -ge 1 ]]; then
   export VOB_BASE_DIR="$1"
   echo "Set VOB_BASE_DIR to $VOB_BASE_DIR ..."
else
   if [[ -z $VOB_BASE_DIR ]]; then
      echo "Error. !!!!!!!VOB_BASE_DIR is not set!!!!!!!"
   else   
      echo VOB_BASE_DIR already defined as $VOB_BASE_DIR ...
   fi
fi

# Set the PV_TOP
#
export PV_TOP=$VOB_BASE_DIR/oscl
echo Set PV_TOP to $PV_TOP ...

export PROJECT=$PV_TOP
echo "Set PROJECT to $PROJECT ..."

export MK="$VOB_BASE_DIR/tools_v2/build/make"
echo "Set MK to $MK ..."

export CCASE_MAKE_COMPAT="gnu"
echo "Set CCASE_MAKE_COMPAT to $CCASE_MAKE_COMPAT ..."

if [[ ! -f "$MK/../bin/archtype" ]];  then
  echo "Error. Cannot find archtype script $MK/../bin/archtype."
fi

export arch_bin_path=`$MK/../bin/archtype`
export extern_tools_path=$VOB_BASE_DIR/extern_tools_v2/bin/$arch_bin_path
export PATH=./$arch_bin_path:$extern_tools_path:$PATH

export arch_bin_path=
export extern_tools_path=

if [[ -f ./setup.extras.ksh ]]; then
   echo File setup.extras.ksh found, sourcing ...
   . ./setup.extras.ksh
else
   echo File setup.extras not found, skipping ...
fi 

echo 
echo Environment is ready if no errors reported
echo

