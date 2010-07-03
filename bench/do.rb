#!/usr/bin/ruby

require 'do_postgres'

c = DataObjects::Connection.new("postgres://127.0.0.1/dbicpp");
st = c.create_command("SELECT * FROM users");

(ARGV[0] || 10000).to_i.times do
  st.execute_reader.each {|r| p r }
end
