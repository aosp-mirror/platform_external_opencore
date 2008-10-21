#!/bin/sh

cd $PV_TOP/../engines/player/test/build/$1

$2 RELEASE=1 ARCHITECTURE=$1 print_pv_libs 2>&1 | grep LIBS | sed -e 's#LIBS *= *##'



