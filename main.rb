require 'sinatra'
require 'sinatra/reloader' if development?
require 'oauth'
require 'json'
require "sinatra/base"
require "sinatra/streaming"
# th = nil

set :sessions, true
enable :sessions

configure :development do
  set :server, :thin
end

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
  if session[:access_token]
    @twitter=Twitter::REST::Client.new do |config|
      config.consumer_key = KEY
      config.consumer_secret = SECRET
      config.access_token = session[:access_token]
      config.access_token_secret = session[:access_token_secret]
    end
  else
    @twitter = nil
  end

end

def base_url
  default_port = (request.scheme == "http") ? 80 : 443
  port = (request.port == default_port) ? "" : ":#{request.port.to_s}"
  "#{request.scheme}://#{request.host}#{port}"
end

get '/'  do
  @title = "Top"
  erb :index
end

post '/server' do
  # $th = Thread.new do
  #   system("./g1g2g3_phone #{params['port']}")
  # end
  $io = IO.popen("./g1g2g3_phone #{params['port']}", "r")
  session[:pid] = $io.pid
  session[:status] = "wait"
  redirect '/'
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
get '/stream_test' do
 stream(:keep_open) do |out|
   out << "It's gonna be legen -\n"
   sleep 5
   out << " (wait for it) \n"
   sleep 10
   out << "- dary!\n"
 end
end

get '/server' do
  # $th = Thread.new do
  #   system("./g1g2g3_phone #{params['port']}")
  # end
  stream do |outp|
    $io=IO.popen("./g1g2g3_phone #{params['port']}", 'r')
    while line=io.gets
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

post '/client' do
  p params
  system("./g1g2g3_phone #{params['ip']} #{params['Cport']}")
end

get '/profile' do 
  @title = "Profile"
  erb :profile
end

get '/products' do 
  @title = "Products"
  erb :products
end

get '/links' do 
  @title = "links"
  erb :links
end

get '/lecture/:folder/:name' do
  html params[:folder]+"/"+params['name']
end

get '/private/:name' do
  html "private/"+params['name']
end

def html(view)
  File.read(File.join('views', "#{view.to_s}.html"))
end
