#!/usr/bin/ruby1.9.1

require 'benchmark'
require_relative 'gems/environment'

class Benchmarks
  @@registry = {}
  def self.register driver, klass
    @@registry[driver] ||= []
    @@registry[driver] << klass
  end

  def self.run driver, sql, n, *args
    puts "\n#{driver}"
    puts "="*driver.to_s.length + "\n"
    Benchmark.bm(30) do |bm|
      @@registry[driver].each do |klass|
        bm.report(klass.name) { klass.new(driver, sql).run(n, *args) }
      end
    end
  end
end

Dir["lib/*.rb"].each {|file| require file }

Benchmarks.run :mysql,      "SELECT SQL_NO_CACHE * FROM users", 5000
Benchmarks.run :postgresql, "SELECT              * FROM users", 5000
