require_relative '../../ruby/dbicpp'

class Benchmarks

  class DBICPP
    attr_reader :driver
    DBI.init File.dirname(__FILE__) + '/../../libs'

    def initialize driver, sql
      @driver = driver
      @h = DBI::Handle.new user: 'udbicpp', db: 'dbicpp', driver: driver
      @sth = @h.prepare sql
    end

    def run n, *args
      open("/dev/null", "w") do |fh|
        n.times do
          @sth.execute(*args) {|r| fh.puts "#{r[:id]} #{r[:name]} #{r[:email]}" }
        end
      end
    end

    def self.name
      "Ruby DBICPP"
    end
  end

  register :mysql, DBICPP
  register :postgresql, DBICPP
end
