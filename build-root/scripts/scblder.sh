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
  if [ -f $1 ]; then
    if [ "X${SBLD_TASKPATH##*/}" == "XCMakeLists.txt" ]; then
      SBLD_TASKPATH=$(dirname $SBLD_TASKPATH)
    fi
  elif [ -d $1 -a -e $1/CMakeLists.txt ]; then
    SBLD_TASKPATH=$(realpath $1)
  else
    echo "FATAL Error, you should follows a path to CMakeLists.txt !!! "
    return
  fi

  local SBLD_TOPDIR=$PWD
  local SBLD_RELATIVE_PATH=${SBLD_TASKPATH##${SBLD_TOPDIR}/src}

  local BLT_DIR=$SBLD_TOPDIR/build-root/${SBLD_RELATIVE_PATH}
  [ -d $BLT_DIR ] || mkdir -p $BLT_DIR
  rm -rf $BLT_DIR/* &>/dev/null
  (
    cd $BLT_DIR
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr ${SBLD_TASKPATH}
    make install
  )
}

scblder $@

