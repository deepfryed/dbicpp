#!/usr/bin/ruby

require 'mkmf'

Config::CONFIG['CC']  = 'g++'
Config::CONFIG['CPP'] = 'g++'

$CFLAGS  = '-I../inc -fPIC -O3'
$LDFLAGS = '-L../libs -ldbic++'

def apt_install_hint pkg
  "sudo apt-get install #{pkg}"
end

def library_installed? name, hint
  if have_library(name)
    true
  else
    $stderr.puts <<-ERROR

      Unable to find required library: #{name}.
      On debian systems, it can be installed as,

      #{hint}

    ERROR
    false
  end
end

exit 1 unless library_installed? 'pcrecpp', apt_install_hint('libpcre3-dev')
exit 1 unless library_installed? 'uuid',    apt_install_hint('uuid-dev')

create_makefile 'dbicpp'
