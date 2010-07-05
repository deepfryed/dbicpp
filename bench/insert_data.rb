#!/usr/bin/ruby

require_relative '../ruby/dbicpp'

DBI.init File.dirname(__FILE__) + '/../libs'

h = DBI::Handle.new user: 'udbicpp', db: 'dbicpp', driver: ARGV[0] || 'postgresql'

h.execute "DROP TABLE IF EXISTS users"
h.execute "CREATE TABLE users (id serial, name text, email text)"

st = h.prepare "INSERT INTO users (name, email) VALUES (?, ?)"

size = (ARGV[1] || 500).to_i

size.times {|n| st.execute("Username #{n}", "email_#{n}@example.com") }
