class Benchmarks

  class LibPQ

    attr_reader :driver

    def initialize driver, sql
      @driver, @sql = driver, sql
      @exec = File.dirname(__FILE__) + '/../pq'
    end

    def run n, *args
      cmd = %Q( #{@exec} --driver="#{@driver}" --sql='#{@sql}' )
      cmd += args.map {|arg| %Q(--bind='#{arg}') }.join(' ')
      cmd += %Q( --num=#{n} )
      system(cmd.strip + " > /dev/null")
    end

    def self.name
      "C libpq"
    end
  end

  register :postgresql, LibPQ
end
