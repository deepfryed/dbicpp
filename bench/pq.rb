#!/usr/bin/ruby

require 'pg'

class Statement
  attr_reader :rows
  def initialize conn, name
    @conn, @name = conn, name
    @rows, @cols, @row = 0, 0 ,0
  end
  def execute *args
    @res  = @conn.exec_prepared @name, args
    @rows = @res.ntuples
    @cols = @res.nfields
  end
  def fetchrow
    if @row < @rows
      @row += 1
      @cols.times.map {|c| @res.getisnull(@row-1,c) ? nil : @res.getvalue(@row-1, c) }
    else
      nil
    end
  end
  def clear
    @res.clear
  end
end

class Connection < PGconn
  def prepare name, sql
    rs = super
    Statement.new(self, name)
  end
end

conn = Connection.connect "host=127.0.0.1 dbname=dbicpp user=udbicpp"
st = conn.prepare "query_bench", "SELECT id, name, email FROM users WHERE id IN ($1, $2)"
(ARGV[0] || 10000).to_i.times do
  st.execute(1,2)
  while r = st.fetchrow
    puts r.join("\t")
  end
  st.clear
end
