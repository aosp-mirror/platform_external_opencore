# makefiles, etc.

echo Cleaning up build environment ...

unset CCASE_MAKE_COMPAT
unset PROJECT
unset SDK_LOCAL
unset VOB_BASE_DIR
unset PV_TOP

if [[ "${MK}" != "" ]]; then
	typeset arch_bin_path=`$MK/../bin/archtype`
	if [ -x /icl-tools/bin/$arch_bin_path/utok ]; then 
   	export PATH=`/icl-tools/bin/$arch_bin_path/utok -s \  -d ./$arch_bin_path  $PATH`
	fi
fi

if [ -f ./cleanup.extras.ksh ]; then
  echo File cleanup.extras.ksh found, sourcing ...
  . ./cleanup.extras.ksh 
else
  echo File cleanup.extras.ksh not found, skipping
fi

unset MK
