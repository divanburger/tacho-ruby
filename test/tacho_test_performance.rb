require 'benchmark'
require 'tacho'

clocks = [:wall, :process, :thread]

passes = 3
iterations = 100_000

def bar(v)
  a = v
  b = v
  c = v
  d = v
  e = v
end

def test
  100_000.times do |i|
    bar(i + 1)
  end
end

clocks.each do |clock_type|
  puts "#{clock_type}"
  puts "=" * clock_type.to_s.length

  tacho_time = 0
  notacho_time = 0

  passes.times do |pass|
    profile = Tacho.start 'test', clock_type: clock_type
    tacho_time += Benchmark.realtime {test}
    profile.stop
    notacho_time += Benchmark.realtime {test}
    puts "Pass #{pass}"
  end

  method_calls = iterations * passes

  total_overhead_time = tacho_time - notacho_time
  puts "  Tacho method time:    #{(tacho_time / method_calls) * 1_000_000_000} ns"
  puts "  No-tacho method time: #{(notacho_time / method_calls) * 1_000_000_000} ns"
  method_overhead_time = (total_overhead_time / method_calls) * 1_000_000_000
  puts "  Per method overhead:  #{method_overhead_time} ns"
end