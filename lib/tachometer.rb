require "tachometer/version"
require "tachometer/tachometer"
require "tachometer/middleware"

module Tachometer
  class Error < StandardError;
  end

  def self.enable(name = '')
    self.start_record("tachometer/#{Time.now.strftime("%Y%m%d_%H%M%S_%N")}.tch", name)
  end

  def self.disable
    self.stop_record
  end
end
