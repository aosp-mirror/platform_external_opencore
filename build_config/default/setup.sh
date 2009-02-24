function print_menu()
{
    echo
    echo "Build selection menu... choose from the following:"
    echo
    echo "1. Build for host platform"
    echo "2. Arm device build using OpenCORE (Android) cross-compiler"
    echo "3. Build using default linux-arm cross-compiler"
    echo "4. Arm device build using OpenCORE (Android) cross-compiler inside AccuRev workspace"
    echo

}

function clean_env()
{
  echo "**********************************"
  echo "Cleaning ARCHITECTURE ..."
  unset ARCHITECTURE
  echo "Cleaning ANDROID_BASE ..."
  unset ANDROID_BASE
  echo "Setting PATH back to the original ..."
  export PATH=$BASE_PATH
  echo "**********************************"
}

function menu()
{
    if [ "$1" ] ; then
        CHOICE=$1
    else
        print_menu
        read -p "Which selection would you like? " CHOICE
    fi

    case $CHOICE in
    1)
        echo "Choice is to build for the host platform."
        clean_env
        ;;
    2)
        echo "Choice is to build for target with OpenCORE (Android) cross-compiler"
        ## clean the environment
        clean_env
        ## set path up for linux OpenCore build
        android_gcc_arm_path=/opt/environments/android/toolchain-eabi-4.2.1/bin
        export ARCHITECTURE=android
        echo "ARCHITECTURE=$ARCHITECTURE"
        export PATH=$android_gcc_arm_path:$BASE_PATH
        export ANDROID_BASE=/opt/environments/android
        echo "ANDROID_BASE=$ANDROID_BASE"
        ;;
    3)
        echo "Choice is to build for target with the default linux-arm cross-compiler"
        # clean the environment
        clean_env
        # set path up for linux-arm compiler
        linux_arm_path=/opt/environments/linux_arm/data/omapts/linux/arm-tc/gcc-3.4.0-1/bin
        export ARCHITECTURE=linux_arm
        export PATH=$linux_arm_path:$BASE_PATH
        ;;
    4)  
        echo "Choice is to build for target with workspace's OpenCORE (Android) cross-compiler"
        ## clean the environment
        clean_env
        ## set path up for linux OpenCore build
        android_gcc_arm_path=$BASE_DIR/toolchains/android/toolchain-eabi-4.2.1/bin
        export ARCHITECTURE=android
        echo "ARCHITECTURE=$ARCHITECTURE"
        export PATH=$android_gcc_arm_path:$BASE_PATH
        export ANDROID_BASE=$BASE_DIR/toolchains/android
        echo "ANDROID_BASE=$ANDROID_BASE"
        ;;
    *)
        echo "Invalid selection.  Please enter your selection again."
        print_menu
        return
        ;;
    esac
}

function mkcmdcmpl()
{
    printf "\nGetting make cmdline completion values...\n"
    export PV_MAKE_COMPLETION_TARGETS=`make -j completion_targets`
    printf "Done getting make cmdline completion values.\n\n"
}



echo Setting up build environment with default configuration ...

export PROJECT_DIR=$PWD

# Set CFG_DIR
#
export CFG_DIR=$PWD
echo "Set CFG_DIR to $CFG_DIR ..."

if [[ $# -ge 1 ]]; then
  export BASE_DIR=${1%/}
  echo "Set BASE_DIR to $BASE_DIR ..."
else
   if [[ -z $BASE_DIR ]]; then
      echo "Error. !!!!!!!BASE_DIR is not set!!!!!!!"
   else   
      echo BASE_DIR already defined as $BASE_DIR ...
   fi
fi

export BUILD_ROOT=$PROJECT_DIR/build
echo Set BUILD_ROOT to $BUILD_ROOT ...

export SRC_ROOT=$BASE_DIR
echo Set SRC_ROOT to $SRC_ROOT ...

export MK=$BASE_DIR/tools_v2/build/make
echo Set MK to $MK ...


extern_tools_path=$BASE_DIR/extern_tools_v2/bin/linux
export PATH=$extern_tools_path:$PATH
export BASE_PATH=$PATH

_pv_make_completion()
{
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    opts="${PV_MAKE_COMPLETION_TARGETS}"

    case "${prev}" in 
      -f)
        COMPREPLY=( $(compgen -f ${cur}) )
        return 0
        ;;
    *)
        COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
        return 0
        ;;
    esac
}

complete -F _pv_make_completion make
###

echo 
echo Environment is ready if no errors reported
echo

