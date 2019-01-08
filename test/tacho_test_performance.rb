require 'benchmark'
require 'tacho'

passes = 3
iterations = 1_000_000
tacho_time = 0
notacho_time = 0

def bar(v)
  a = v
end

def test_tacho
  profile = Tacho.start 'test'
  1_000_000.times do |i|
    bar(i + 1)
  end
  profile.stop
end

def test_notacho
  1_000_000.times do |i|
    bar(i + 1)
  end
end

passes.times do
  puts 'Tacho:'
  time = Benchmark.realtime { test_tacho }
  puts time
  tacho_time += time
  puts '------'
  puts 'No tacho:'
  time = Benchmark.realtime { test_notacho }
  puts time
  notacho_time += time
  puts '------'
end

total_overhead_time = tacho_time - notacho_time
puts "Total overhead: #{total_overhead_time} s"
puts "Percentage overhead: #{100.0 - notacho_time * 100 / tacho_time} %"
method_overhead_time = (total_overhead_time / iterations) * 1000000
puts "Per method overhead: #{method_overhead_time} us"