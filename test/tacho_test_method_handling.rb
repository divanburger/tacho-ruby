require 'tacho'

def rec_bar(value)
  if value == 0
    a = 1
  else
    rec_bar(value - 1)
  end
end

def rec_foo
  rec_bar(100)
end

def iter_bar
  a = 1
end

def iter_foo
  1_000.times do |i|
    iter_bar
  end
end

def iter_rec_bar(value)
  if value == 0
    a = 1
  else
    iter_rec_bar(value - 1)
    iter_rec_bar(value - 1)
  end
end

def iter_rec_foo
  100.times do |i|
    iter_rec_bar(10)
  end
end

def proc_bar
  bop = -> do
    iter_bar
  end

  baz = -> do
    a = 1
    bop.call
  end

  iter_bar
  baz.call
  baz.call
end

def proc_foo
  bar = -> do
    proc_bar
    proc_bar
  end

  bar.call
  iter_bar
  bar.call
end

def block_baz
  iter_bar
end

def block_bar
  iter_bar
  yield
  iter_bar
end

def block_foo
  block_bar do
    block_baz
  end
end

def complex_bar(n)
  if n == 0
    a = 1
  else
    complex_bar(n - 1)
  end
end

def complex_baz
  [*1..100]
end

def complex_foo
  complex_baz.select {|n| n < 12}.reject {|n| n < 5}.each do |n|
    complex_bar(n)
  end
end

module ActiveSupport
  module Tryable #:nodoc:
    def try(*a, &b)
      try!(*a, &b) if a.empty? || respond_to?(a.first)
    end

    def try!(*a, &b)
      if a.empty? && block_given?
        if b.arity == 0
          instance_eval(&b)
        else
          yield self
        end
      else
        public_send(*a, &b)
      end
    end
  end
end

class Object
  include ActiveSupport::Tryable
end

class Test

  def foo
    bar
  end

  def bar
    self
  end

end

def try_foo
  a = Test.new
  a.try(:bar).try(:foo).try(:bar)
end

# trace = TracePoint.new do |tp|
#   puts "#{tp.event} #{tp.path}:#{tp.lineno}: #{tp.method_id} #{tp.callee_id}"
# end
# trace.enable

profile = Tacho.start 'test', clock_type: :thread
# iter_foo
# rec_foo
# iter_rec_foo
# proc_foo
# block_foo
# complex_foo
try_foo
profile.stop

# trace.disable