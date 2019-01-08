module Tacho
  class Middleware
    def initialize app
      @app = app
    end

    def call(env)
      Tacho.enable
      response = @app.call(env)
      Tacho.disable
      response
    end
  end
end