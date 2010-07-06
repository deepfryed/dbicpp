#!/bin/bash

rm -rf lib/*
rm -rf cmake_install.cmake CMakeFiles/ CMakeCache.txt install_manifest.txt Makefile
cmake -DCMAKE_INSTALL_PREFIX:PATH=.
make
make install
make clean
rm -rf cmake_install.cmake CMakeFiles/ CMakeCache.txt install_manifest.txt Makefile
