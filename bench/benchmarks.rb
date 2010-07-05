#!/usr/bin/ruby

require_relative '.gems/environment'
require 'benchmark'

class Benchmarks
  @@registry = {}
  def self.register driver, klass
    @@registry[driver] ||= []
    @@registry[driver] << klass
  end

  def self.run driver, sql, n, *args
    Benchmark.bm(30) do |bm|
      @@registry[driver].each do |klass|
        name = klass.name
        bm.report("#{name}/#{driver}") { klass.new(driver, sql).run(n, *args) }
      end
    end
  end
end

Dir["lib/*.rb"].each {|file| require file }

Benchmarks.run :mysql,      "SELECT SQL_NO_CACHE * FROM users", 10000
Benchmarks.run :postgresql, "SELECT              * FROM users", 10000
