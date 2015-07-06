require 'sinatra'
require 'sinatra/reloader' if development?
require 'oauth'
require 'json'
require "sinatra/base"
require "sinatra/streaming"
require 'sinatra-websocket'
# th = nil

set :sessions, true
enable :sessions
set :server, 'thin'
set :sockets, []


helpers do
  include Rack::Utils
  alias_method :h, :escape_html
end

configure do
  use Rack::Session::Cookie, :secret => Digest::SHA1.hexdigest(rand.to_s)
  KEY = "zoNWH4JyeCgYuaBi88fwAp6Sw"
  SECRET = "46CMT4LCRQ5Fso2UUOv2nIVzXVuZr2IXDbR2KABlhkLyxht2bL"
end

before do

end


get '/'  do
  @title = "Top"
  erb :index
end

post '/server' do
  # $th = Thread.new do
  #   system("./g1g2g3_phone #{params['port']}")
  # end
  $io = IO.popen("./g1g2g3_phone_chat #{params['port']}", "r")
  session[:pid] = $io.pid
  session[:status] = "wait"
  redirect '/talk'
end

post '/client' do
  p params
  system("./g1g2g3_phone_chat #{params['port']} #{params['ip']}")
  $io = IO.popen("./g1g2g3_phone_chat #{params['port']}", "r")
  session[:pid] = $io.pid
  session[:status] = "wait"
  redirect '/talk'
end

get '/sendrecv' do
  if !request.websocket?
    erb :index
  else
    puts "sendrecv"
    request.websocket do |ws|
      ws.onopen do
        ws.send("Hello World!")
        settings.sockets << ws
      end
      ws.onmessage do |msg|
        EM.next_tick { settings.sockets.each{|s| s.send(msg) } }
      end
      ws.onclose do
        warn("websocket closed")
        settings.sockets.delete(ws)
      end
    end
  end
end

get '/talk' do 
  erb :talk
end

get '/client_test' do
  stream do |outp|
    $io=IO.popen("./client_recv 192.168.0.4 3800", 'r')
    while line=io.gets
      puts line
      outp << line
    end
  end
end


get '/server' do
  # $th = Thread.new do
  #   system("./g1g2g3_phone #{params['port']}")
  # end
  stream do |outp|
    $io=IO.popen("./g1g2g3_phone_chat #{params['port']}", 'r')
    while line=$io.gets
      puts line
      outp << line
    end
  end
  # $io = IO.popen("./g1g2g3_phone #{params['port']}", "r")
  session[:pid] = $io.pid
  session[:status] = "wait"
  erb :index
end

get '/stop' do 
  p $io
  p session
  Process.kill("KILL", session[:pid]) #stop the system process
  # $io.close
  # Thread.kill($th)
  p $io
end

