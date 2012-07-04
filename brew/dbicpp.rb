require 'formula'

class Dbicpp < Formula
  url 'https://github.com/deepfryed/dbicpp/tarball/OSX-0.6.2'
  homepage 'https://github.com/deepfryed/dbicpp'
  version '0.6.2'
  md5 '3d41340844fc944d83fa7ee5fd8f9bd8'

  depends_on 'cmake'
  depends_on 'ossp-uuid'
  depends_on 'pcre'
  depends_on 'sqlite3'

  def install
    version = '1.3'
    flags   = %w(PG MYSQL SQLITE3).map {|name| "-DCMAKE_#{name}_VERSION=#{version}"}.join(' ')
    system "cmake #{flags} -DCMAKE_INSTALL_PREFIX:PATH=#{prefix}"
    system "make install"
  end
end
