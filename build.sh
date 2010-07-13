#!/bin/bash

export PG_VERSION=$(cat src/drivers/pg.cc | grep "VERSION \+" | sed 's/^.*VERSION *//' | sed 's/"//g')
export MYSQL_VERSION=$(cat src/drivers/mysql.cc | grep "VERSION \+" | sed 's/^.*VERSION *//' | sed 's/"//g')

cleanup() {
  make clean
  rm -rf cmake_install.cmake CMakeFiles/ CMakeCache.txt install_manifest.txt Makefile
  rm -rf debian/dbic++-dev.debhelper.log
  rm -rf debian/dbic++-dev.substvars
  rm -rf debian/dbic++-dev/
  rm -rf debian/dbic++-mysql.debhelper.log
  rm -rf debian/dbic++-mysql.postinst.debhelper
  rm -rf debian/dbic++-mysql.postrm.debhelper
  rm -rf debian/dbic++-mysql.substvars
  rm -rf debian/dbic++-mysql/
  rm -rf debian/dbic++-pg.debhelper.log
  rm -rf debian/dbic++-pg.postinst.debhelper
  rm -rf debian/dbic++-pg.postrm.debhelper
  rm -rf debian/dbic++-pg.substvars
  rm -rf debian/dbic++-pg/
  rm -rf debian/files
  rm -rf debian/stamp-autotools-files
  rm -rf debian/stamp-makefile-build
  rm -rf debian/tmp/
}

usage() {
  echo "

    $0 [options]
    
    -h print this help message.
    -d builds debian packages.
    -l builds and install libraries locally into lib (default).
    -c cleanup all temporary files.
    -i builds and installs dbic++ under /usr (root)
    -u uninstall dbic++ stuff from /usr (root)

  "
}

_uninstall() {
  rm -rf /usr/lib/libdbic++.a
  rm -rf /usr/lib/dbic++
  rm -rf /usr/include/dbic++*
}

_install() {
  uninstall
  cmake -DCMAKE_PG_VERSION=$PG_VERSION -DCMAKE_MYSQL_VERSION=$MYSQL_VERSION -DCMAKE_INSTALL_PREFIX:PATH=/usr
  make
  make install
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

while getopts "cdhilu" OPTION
do
  case $OPTION in
    c) cleanup; exit 0;;
    d) debian_build; exit 0;;
    i) local_build; exit 0;;
    i) _install; exit 0;;
    u) _uninstall; exit 0;;
    h) usage; exit 0;;
    ?) usage; exit 0;;
  esac
done

local_build
