require "tachometer/version"
require "tachometer/tachometer"
require "tachometer/middleware"

module Tachometer
  class Error < StandardError;
  end

  def self.start(name = '')
    profile = Profile.new()
    profile.start("tachometer/#{Time.now.strftime("%Y%m%d_%H%M%S_%N")}.tch", name)
    return profile
  end
end
