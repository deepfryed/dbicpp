class Benchmarks

  class LibDBICPP

    attr_reader :driver

    def initialize driver, sql
      @driver, @sql = driver, sql
      @exec = File.dirname(__FILE__) + '/../bin/dbicpp'
    end

    def run n, *args
      cmd = %Q( #{@exec} --driver="#{@driver}" --sql='#{@sql}' )
      cmd += args.map {|arg| %Q(--bind='#{arg}') }.join(' ')
      cmd += %Q( --num=#{n} )
      system(cmd.strip + " > /dev/null")
    end

    def self.name
      "C++ DBICPP"
    end
  end

  register :mysql, LibDBICPP
  register :postgresql, LibDBICPP
end
