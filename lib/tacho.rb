require "tacho/version"
require "tacho/tacho"
require "tacho/middleware"

module Tacho
  class Error < StandardError;
  end

  def self.start(name = '')
    profile = Profile.new()
    profile.start("tacho/#{Time.now.strftime("%Y%m%d_%H%M%S_%N")}.tch", name)
    return profile
  end
end
