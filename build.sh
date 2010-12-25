#!/bin/bash

export PG_VERSION=$(cat src/drivers/pg/common.h | grep "VERSION \+" | sed 's/^.*VERSION *//' | sed 's/"//g')
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
  rm -rf Data/ Languages.txt Topics.txt Menu.txt
}

realclean() {
  cleanup
  rm -rf lib/*
  rm -rf bench/bin/*
  rm -rf bench/src/*.o src/*.o src/drivers/*.o src/drivers/pg/*.o
}

builddocs() {
  rm -rf Data/
  rm -rf doc/{files2,index,index.html,javascript,search,styles,dbicpp*}

  naturaldocs -i src/ -i inc/ -o HTML doc/ -s doc/Web -p .

  if [ -e doc/files2/dbic++-h.html ]; then
    mv doc/files2/dbic++-h.html doc/files2/dbicpp-h.html
  fi
  if [ -d doc/files2/dbic++ ]; then
    mv doc/files2/dbic++ doc/files2/dbicpp
  fi

  for file in `grep -lr "dbic++/"   doc/`; do sed -i 's/dbic++\//dbicpp\//g' $file; done
  for file in `grep -lr "dbic++-h"  doc/`; do sed -i 's/dbic++-h/dbicpp-h/g' $file; done
  for file in `grep -lr "../files2" doc/`; do sed -i 's/..\/files2/../g'     $file; done

  mv doc/files2/* doc/
  for file in `grep -lr "../../"    doc/dbicpp`;        do sed -i 's/\.\.\/\.\./../g' $file; done
  for file in `grep -lr "../"       doc/*.html`; do sed -i 's/\.\.\///g'       $file; done

  rm -f doc/index.html
  cd doc && ln -s about-txt.html index.html && cd ..

  for file in `find doc/ -type f -name "*.html"`; do
    echo '<a href="http://github.com/deepfryed/dbicpp"><img style="position: absolute; top: 0; right: 0; border: 0;" src="http://s3.amazonaws.com/github/ribbons/forkme_right_orange_ff7600.png" alt="Fork me on GitHub" /></a>' >> $file
  done
}

usage() {
  echo "

    $0 [options]

    -h print this help message.
    -d builds debian binary packages for local architecture.
    -s builds debian source packages for this version.
    -l builds and install libraries locally into lib (default).
    -c cleanup all temporary files.
    -i builds and installs dbic++ under /usr (root)
    -u uninstall dbic++ stuff from /usr (root)
    -r real clean - cleanups up build artifacts and local install files
    -g generate HTML documentation using naturaldocs

  "
}

_uninstall() {
  rm -rf /usr/lib/libdbic++.a
  rm -rf /usr/lib/dbic++
  rm -rf /usr/include/dbic++*
}

_install() {
  _uninstall
  cmake -DCMAKE_PG_VERSION=$PG_VERSION \
        -DCMAKE_MYSQL_VERSION=$MYSQL_VERSION \
        -DCMAKE_DB2_VERSION=$DB2_VERSION \
        -DCMAKE_INSTALL_PREFIX:PATH=/usr
  make
  make install
}

debian_binary_build() {
  dpkg-buildpackage -rfakeroot -b
  make clean
}

debian_source_build() {
  PREFIX=dbic++-$(cat debian/changelog | head -n1 | grep -o "[0-9.]\+")
  git archive --remote=$PWD --format=tar --prefix=$PREFIX/ HEAD | tar -C $PWD/.. -xvf -
  cd $PWD/../$PREFIX
  debuild -S -sa
}

local_build() {
  cmake -DCMAKE_PG_VERSION=$PG_VERSION \
        -DCMAKE_MYSQL_VERSION=$MYSQL_VERSION \
        -DCMAKE_DB2_VERSION=$DB2_VERSION \
        -DCMAKE_INSTALL_PREFIX:PATH=tmp/
  make
  make install
  rm -rf lib/*
  mv tmp/lib/* lib/
  rm -rf tmp/*
  rm -f install_manifest.txt
}

while getopts "cdhilursg" OPTION
do
  case $OPTION in
    c) cleanup; exit 0;;
    d) debian_binary_build; exit 0;;
    l) local_build; exit 0;;
    i) _install; exit 0;;
    u) _uninstall; exit 0;;
    r) realclean; exit 0;;
    s) debian_source_build; exit 0;;
    h) usage; exit 0;;
    g) builddocs; exit 0;;
    ?) usage; exit 0;;
  esac
done

local_build
