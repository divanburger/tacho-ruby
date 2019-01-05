module Tachometer
  class Middleware
    def initialize app
      @app = app
    end

    def call(env)
      Tachometer.enable
      response = @app.call(env)
      Tachometer.disable
      response
    end
  end
end