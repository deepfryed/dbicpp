#!/bin/bash

rm -rf cmake_install.cmake CMakeFiles/ CMakeCache.txt 
cmake -DCMAKE_INSTALL_PREFIX:PATH=.
make
make install
make clean
rm -rf cmake_install.cmake CMakeFiles/ CMakeCache.txt 
