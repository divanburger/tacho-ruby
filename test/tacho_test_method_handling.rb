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
  1_000_000.times do |i|
    iter_bar
  end
end

profile = Tacho.start 'test'
iter_foo
rec_foo
profile.stop