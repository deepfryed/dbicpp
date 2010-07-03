#!/usr/bin/ruby

require_relative '../ruby/dbicpp'

DBI.init File.dirname(__FILE__) + '/../libs'

h = DBI::Handle.new user: 'udbicpp', db: 'dbicpp', driver: 'postgresql'

st = h.prepare "SELECT * FROM users WHERE id IN (?, ?)"

(ARGV[0] || 10000).to_i.times {
  st.execute(1,2)
  while r = st.fetchrow
    p r
  end
}
