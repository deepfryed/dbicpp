#!/bin/bash

OPERATION="local-build"
export PG_VERSION=$(cat src/drivers/pg.cc | grep "VERSION \+" | sed 's/^.*VERSION *//' | sed 's/"//g')
export MYSQL_VERSION=$(cat src/drivers/mysql.cc | grep "VERSION \+" | sed 's/^.*VERSION *//' | sed 's/"//g')

cleanup() {
  make clean
  rm -rf cmake_install.cmake CMakeFiles/ CMakeCache.txt install_manifest.txt Makefile
  rm -rf debian/dbicpp-dev.debhelper.log
  rm -rf debian/dbicpp-dev.substvars
  rm -rf debian/dbicpp-dev/
  rm -rf debian/dbicpp-mysql.debhelper.log
  rm -rf debian/dbicpp-mysql.postinst.debhelper
  rm -rf debian/dbicpp-mysql.postrm.debhelper
  rm -rf debian/dbicpp-mysql.substvars
  rm -rf debian/dbicpp-mysql/
  rm -rf debian/dbicpp-pg.debhelper.log
  rm -rf debian/dbicpp-pg.postinst.debhelper
  rm -rf debian/dbicpp-pg.postrm.debhelper
  rm -rf debian/dbicpp-pg.substvars
  rm -rf debian/dbicpp-pg/
  rm -rf debian/files
  rm -rf debian/stamp-autotools-files
  rm -rf debian/stamp-makefile-build
  rm -rf debian/tmp/
}

usage() {
echo <<-EOF

    $0 [options]
    
    -h print this help message.
    -d builds debian packages.
    -l builds and install libraries locally into lib (default).
    -c cleanup all temporary files.

EOF
  exit
}

debian_build() {
  dpkg-buildpackage -rfakeroot -b
  make clean
}

local_build() {
  cmake -DCMAKE_PG_VERSION=$PG_VERSION -DCMAKE_MYSQL_VERSION=$MYSQL_VERSION -DCMAKE_INSTALL_PREFIX:PATH=tmp/
  make
  make install
  rm -rf lib/*
  mv tmp/lib/* lib/
  rm -rf tmp/*
  rm -f install_manifest.txt
}

while getopts "dlchp:" OPTION
do
  case $OPTION in
    h) usage;;
    d) OPERATION="debian-build";;
    c) OPERATION="cleanup";;
    ?) usage;;
  esac
done

case $OPERATION in
  "debian-build") debian_build;;
  "local-build") local_build;;
  "cleanup") cleanup;;
  ?) usage;;
esac
exit 0
