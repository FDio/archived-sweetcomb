#!/usr/bin/env bash
#=================================================================
# CPSTR: Copyright (c) 2019 By Abodu, All Rights Reserved.
# FNAME: scblder.sh
# AUTHR: abodu,abodu@qq.com
# CREAT: 2019-01-17 11:07:51
# ENCOD: UTF-8 Without BOM
# VERNO: 0.0.1
# LUPTS: 2019-01-17 11:12:45
#=================================================================

scblder() {
  if [ "X$1" == "X" -o "X$1" == "X-h" -o "X$1" == "X--help" ]; then
    echo
    echo "Usage: $FUNCNAME <path/to/CMakeLists.txt>"
    echo
    return
  fi

  local SBLD_TASKPATH=$(realpath $1)
  if [ -f $SBLD_TASKPATH ]; then
    if [ "X${SBLD_TASKPATH##*/}" == "XCMakeLists.txt" ]; then
      SBLD_TASKPATH=$(dirname $SBLD_TASKPATH)
    else
      echo "ERROR, please provides an available 'CMakeLists.txt' !!! "
    fi
  else
    if [ -d $SBLD_TASKPATH -a ! -e $SBLD_TASKPATH/CMakeLists.txt ]; then
      echo "ERROR, '$SBLD_TASKPATH' not contains CMakeLists.txt, please check and re-run !!!"
      return
    fi
  fi

  local SBLD_TOPDIR=$PWD
  local SBLD_RELATIVE_PATH=${SBLD_TASKPATH##${SBLD_TOPDIR}/src}

  local BLT_DIR=$SBLD_TOPDIR/build-root/${SBLD_RELATIVE_PATH}
  [ -d $BLT_DIR ] || mkdir -p $BLT_DIR
  (
    cd $BLT_DIR
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr ${SBLD_TASKPATH}
    make install
  )
}

scblder $@
