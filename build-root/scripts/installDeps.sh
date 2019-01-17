#!/usr/bin/env bash
#=================================================================
# CPSTR: Copyright (c) 2019 By Abodu, All Rights Reserved.
# FNAME: installDeps.sh
# AUTHR: abodu,abodu@qq.com
# CREAT: 2019-01-17 15:50:31
# ENCOD: UTF-8 Without BOM
# VERNO: 0.0.1
# LUPTS: 2019-01-17 15:50:31
#=================================================================

bld_yumInstallBases() {
  local PACKS="centos-release-scl epel-release "
  PACKS="${PACKS} git vim doxygen wget curl devtoolset-7"
  PACKS="${PACKS} rpm-build pkgconfig automake cmake bison flex "
  PACKS="${PACKS} protobuf-devel protobuf-c-devel curl-devel"
  PACKS="${PACKS} pcre-devel python-devel python34-devel lua-devel"
  PACKS="${PACKS} zlib-devel libcmocka-devel libev-devel"
  PACKS="${PACKS} openssl-devel libssh-devel libssh2-devel"
  yum install -y $(echo $PACKS)
  scl enable devtoolset-7 bash
}

bld_yumInstallBases $@

