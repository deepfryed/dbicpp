require 'mysql2'

class Benchmarks

  class MySQL2
    attr_reader :driver
    def initialize driver, sql
      @driver, @sql = driver, sql
      @h = Mysql2::Client.new host: "localhost", username: "udbicpp", database: "dbicpp"
    end

    def run n, *args
      sql = process_command(sql, args)
      open("/dev/null", "w") do |fh|
        n.times do
          @h.query(sql).each {|r| fh.puts "#{r['id']} #{r['name']} #{r['email']}" }
        end
      end
    end

    def process_command sql, args
      sql = @sql.dup
      while arg = args.shift
        sql.sub!(/(?<!')\?(?!')/, arg.to_s)
      end
      sql
    end

    def self.name
      "ruby: mysql2"
    end
  end

  register :mysql, MySQL2
end
