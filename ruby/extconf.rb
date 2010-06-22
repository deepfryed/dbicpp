#!/usr/bin/ruby

require 'mkmf'

Config::CONFIG['CC']  = 'g++'
Config::CONFIG['CPP'] = 'g++'

$CFLAGS = "-I../inc -fPIC"
$LDFLAGS = "-L../libs -lpcrecpp -ldl -ldbic++"

create_makefile 'dbicpp'
