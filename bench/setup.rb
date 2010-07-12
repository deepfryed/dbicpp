#!/usr/bin/ruby

require 'swift'
require 'etc'

Swift::DBI.init File.dirname(__FILE__) + '/../../dbicpp/lib/dbic++'

h = Swift::DBI::Handle.new user: Etc.getlogin, db: 'dbicpp', driver: ARGV[0] || 'postgresql'

h.execute "DROP TABLE IF EXISTS users"
h.execute "CREATE TABLE users (id serial, name text, email text)"

st = h.prepare "INSERT INTO users (name, email) VALUES (?, ?)"

size = (ARGV[1] || 500).to_i

size.times {|n| st.execute("Username #{n + 1}", "email_#{n + 1}@example.com") }
