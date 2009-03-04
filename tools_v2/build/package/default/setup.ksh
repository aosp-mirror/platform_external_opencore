echo Setting up build environment with default configuration ...
echo Using $PWD as top directory ...


# Setup environment for necessary for the
# makefiles, etc.

# Only set the VOB_BASE_DIR if it is not already set.
# In this case, set it based on the first argument if it is present
# and otherwise default to /vobs.
if [[ -z $VOB_BASE_DIR ]]; then
   if [[ $# -ge 1 ]]; then 
     export VOB_BASE_DIR="$1"
   else 
     export VOB_BASE_DIR="/vobs"
   fi
   echo "Set VOB_BASE_DIR to $VOB_BASE_DIR ..."
else
   echo VOB_BASE_DIR already defined as $VOB_BASE_DIR ...
fi

# Set the PV_TOP
#
export PV_TOP="$VOB_BASE_DIR/oscl"
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
if [[ -x /icl-tools/bin/$arch_bin_path/utok ]]; then
  export PATH=$PATH:`/icl-tools/bin/$arch_bin_path/utok -s \  ./$arch_bin_path  $extern_tools_path $PATH`
else
  export PATH=./$arch_bin_path:$extern_tools_path:$PATH
fi
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
