module Swift; end
require 'swift/ext/swift/dbi'

class Benchmarks

  class DBICPP
    attr_reader :driver
    Swift::DBI.init File.dirname(__FILE__) + '/../../lib/dbic++'

    def initialize driver, sql
      @driver = driver
      @h = Swift::DBI::Handle.new user: 'udbicpp', db: 'dbicpp', driver: driver
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
      "ruby: DBICPP"
    end
  end

  register :mysql, DBICPP
  register :postgresql, DBICPP
end
