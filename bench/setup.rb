#!/usr/bin/ruby1.9.1

require 'swift'

Swift.init File.dirname(__FILE__) + '/../../dbicpp/lib/dbic++'

h = Swift::Adapter.new db: 'dbicpp', driver: ARGV[0] || 'postgresql'

h.execute "drop table if exists users"
h.execute "create table users (id serial, name text, email text)"

st = h.prepare "insert into users (name, email) values (?, ?)"

size = (ARGV[1] || 500).to_i

size.times {|n| st.execute("Username #{n + 1}", "email_#{n + 1}@example.com") }
