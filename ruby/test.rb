#!/usr/bin/ruby

require_relative 'dbicpp'

DBI.init File.dirname(__FILE__) + '/../libs'

h = DBI::Handle.new user: 'bharanee', db: 'dbicpp', driver: 'postgresql'

st = h.prepare "SELECT * FROM users WHERE id > ?"
st.execute(0) {|r| p r }
p st.execute(2).first
p st.execute(2).fetchrow
