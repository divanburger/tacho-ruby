require 'tacho'
require 'tracer'

def too_high
  raise "Error!"
end

def test(value)
  if value > 0
    too_high
  else
    a = 2
  end
end

def test_retry(value)

  begin
    test(value)

  rescue
    if value > 0
      value -= 1
      retry
    else
      raise 'Unknown error'
    end
  end

end

def test_noretry(value)

  begin
    test(value)

  rescue
    puts 'Ah well'
  end

end

def foo
  # test_noretry(2)
  test_retry(2)
end

# stack = []
#
# trace = TracePoint.new(:line, :class, :end, :call, :return, :c_call, :c_return, :raise, :b_call, :b_return, :thread_begin, :thread_end, :fiber_switch) do |tp|
#   puts "#{tp.event} #{tp.path}:#{tp.lineno}: #{tp.method_id} #{tp.callee_id}"
#
#   case tp.event
#   when :call, :c_call
#     stack << tp.method_id
#   when :return, :c_return
#     stack.pop
#   end
#   puts stack.join(', ')
#
# end
# trace.enable

profile = Tacho.start 'test', clock_type: :thread
foo
profile.stop

# trace.disable