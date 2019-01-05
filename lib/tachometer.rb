require "tachometer/version"
require "tachometer/tachometer"
require "tachometer/middleware"

module Tachometer
  class Error < StandardError;
  end

  def self.enable(name = '')
    self.start_record("tachometer/#{Time.now.strftime("%Y%m%d_%H%M%S_%N")}.tch", name)
    @tracepoint = TracePoint.new(:call, :return) {|tp| self.record(tp.event.to_s, tp.path, tp.lineno, tp.callee_id.to_s)}
    @tracepoint.enable
  end

  def self.disable
    @tracepoint.disable
    self.stop_record
  end
end
