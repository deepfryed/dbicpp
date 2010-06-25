#!/usr/bin/ruby

require_relative 'dbicpp'
require 'pathname'

DBI.init Pathname.new(__FILE__).dirname + '..' + 'libs'
DBI.trace true

h = DBI::Handle.new user: 'udbicpp', db: 'dbicpp', driver: 'postgresql'

st = h.prepare "SELECT * FROM users WHERE id > ?"

puts "\nSELECT and print results"
puts "------------------------\n"

st.execute(0) {|r| p r }

puts "\nSELECT and print first row"
puts "--------------------------\n"
p st.execute(2).first

puts "\nSELECT and print first row as array"
puts "-----------------------------------\n"
p st.execute(2).fetchrow

puts "\nNested transactions"
puts "===================\n"

h.transaction {

  puts "\nDelete user id = 2"
  puts "------------------\n"
  h.execute("DELETE FROM users WHERE id = 2");

  begin
    h.transaction {
      puts "\nDelete all users"
      puts "----------------\n"
      h.execute("DELETE FROM users");
      raise "Raise error deleting all users"
    }
  rescue => e
    puts "#{e}"
  end
}

puts "\nSELECT and print results"
puts "------------------------\n"

st.execute(0) {|r| p r }
