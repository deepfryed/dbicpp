#!/usr/bin/ruby

require '.gems/environment'

require 'do_mysql'
require 'mysql2'
require 'benchmark'
require_relative '../ruby/dbicpp'

DBI.init File.dirname(__FILE__) + '/../libs'

dbicpp_h   = DBI::Handle.new user: 'udbicpp', db: 'dbicpp', driver: 'mysql'
dbicpp_sth = dbicpp_h.prepare 'SELECT SQL_NO_CACHE * FROM users'

do_h   = DataObjects::Connection.new("mysql://127.0.0.1/dbicpp");
do_sth = do_h.create_command("SELECT SQL_NO_CACHE * FROM users");

mysql2_h = Mysql2::Client.new host: "localhost", username: "udbicpp", database: "dbicpp"

n = (ARGV[0] || 10000).to_i

Benchmark.bm(14) do |bm|
  bm.report("mysql2") do
    n.times do
      res = mysql2_h.query "SELECT SQL_NO_CACHE * FROM users"
      res.each {|r| $stderr.puts r.inspect }
    end
  end
  bm.report("do_mysql") do
    n.times do
      res = do_sth.execute_reader
      res.each {|r| $stderr.puts r.inspect }
    end
  end
  bm.report("dbicpp#next") do
    n.times do
      dbicpp_sth.execute
      while r = dbicpp_sth.next
        $stderr.puts r.inspect
      end
    end
  end
  bm.report("dbicpp#each") do
    n.times do
      dbicpp_sth.execute {|r| $stderr.puts r.inspect }
    end
  end
end
