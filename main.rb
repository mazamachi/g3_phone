require 'sinatra'
require 'sinatra/reloader' if development?
require 'oauth'
require 'json'
require "sinatra/base"
require "sinatra/streaming"
require 'sinatra-websocket'
require 'active_record'
load "translate.rb"
# th = nil

set :sessions, true
enable :sessions
set :server, 'thin'
set :sockets, []
LANG_CODES = {
"Bulgarian"=> {web:"bg", ms: "bg" },
"Catalan"=> {web:"ca", ms: "ca" },
"Arabic (Egypt)"=> {web:"ar-EG", ms: "ar" },
"Arabic (Jordan)"=> {web:"ar-JO", ms: "ar" },
"Arabic (Kuwait)"=> {web:"ar-KW", ms: "ar" },
"Arabic (Lebanon)"=> {web:"ar-LB", ms: "ar" },
"Arabic (Qatar)"=> {web:"ar-QA", ms: "ar" },
"Arabic (UAE)"=> {web:"ar-AE", ms: "ar" },
"Arabic (Morocco)"=> {web:"ar-MA", ms: "ar" },
"Arabic (Iraq)"=> {web:"ar-IQ", ms: "ar" },
"Arabic (Algeria)"=> {web:"ar-DZ", ms: "ar" },
"Arabic (Bahrain)"=> {web:"ar-BH", ms: "ar" },
"Arabic (Lybia)"=> {web:"ar-LY", ms: "ar" },
"Arabic (Oman)"=> {web:"ar-OM", ms: "ar" },
"Arabic (Saudi Arabia)"=> {web:"ar-SA", ms: "ar" },
"Arabic (Tunisia)"=> {web:"ar-TN", ms: "ar" },
"Arabic (Yemen)"=> {web:"ar-YE", ms: "ar" },
"Czech"=> {web:"cs", ms: "cs" },
"Dutch"=> {web:"nl-NL", ms: "nl" },
"English (Australia)"=> {web:"en-AU", ms: "en" },
"English (Canada)"=> {web:"en-CA", ms: "en" },
"English (India)"=> {web:"en-IN", ms: "en" },
"English (New Zealand)"=> {web:"en-NZ", ms: "en" },
"English (South Africa)"=> {web:"en-ZA", ms: "en" },
"English(UK)"=> {web:"en-GB", ms: "en" },
"English(US)"=> {web:"en-US", ms: "en" },
"Finnish"=> {web:"fi", ms: "fi" },
"French"=> {web:"fr-FR", ms: "fr" },
"German"=> {web:"de-DE", ms: "de" },
"Hebrew"=> {web:"he", ms: "he" },
"Hungarian"=> {web:"hu", ms: "hu" },
"Italian"=> {web:"it-IT", ms: "it" },
"Indonesian"=> {web:"id", ms: "id" },
"Japanese"=> {web:"ja", ms: "ja" },
"Korean"=> {web:"ko", ms: "ko" },
"Mandarin Chinese"=> {web:"zh-CN", ms: "zh-CHS" },
"Traditional Taiwan"=> {web:"zh-TW", ms: "zh-CHS" },
"Simplified China"=> {web:"zh-CN", ms: "zh-CHS" },
"Simplified Hong Kong"=> {web:"zh-HK", ms: "zh-CHS" },
"Yue Chinese (Traditional Hong Kong)"=> {web:"zh-yue", ms: "zh-CHS" },
"Malaysian"=> {web:"ms-MY", ms:"ms" },
"Norwegian"=> {web:"no-NO", ms: "no" },
"Polish"=> {web:"pl", ms: "pl" },
"Portuguese"=> {web:"pt-PT", ms: "pt" },
"Portuguese (brasil)"=> {web:"pt-BR", ms: "pt" },
"Romanian"=> {web:"ro-RO", ms: "ro" },
"Russian"=> {web:"ru", ms: "ru" },
"Serbian"=> {web:"sr-SP", ms: "sr-Cyrl" },
"Slovak"=> {web:"sk", ms: "sk" },
"Spanish(Argentina)"=> {web:"es-AR", ms: "es" },
"Spanish(Bolivia)"=> {web:"es-BO", ms: "es" },
"Spanish(Chile)"=> {web:"es-CL", ms: "es" },
"Spanish(Colombia)"=> {web:"es-CO", ms: "es" },
"Spanish(Costa Rica)"=> {web:"es-CR", ms: "es" },
"Spanish(Dominican Republic)"=> {web:"es-DO", ms: "es" },
"Spanish(Ecuador)"=> {web:"es-EC", ms: "es" },
"Spanish(El Salvador)"=> {web:"es-SV", ms: "es" },
"Spanish(Guatemala)"=> {web:"es-GT", ms: "es" },
"Spanish(Honduras)"=> {web:"es-HN", ms: "es" },
"Spanish(Mexico)"=> {web:"es-MX", ms: "es" },
"Spanish(Nicaragua)"=> {web:"es-NI", ms: "es" },
"Spanish(Panama)"=> {web:"es-PA", ms: "es" },
"Spanish(Paraguay)"=> {web:"es-PY", ms: "es" },
"Spanish(Peru)"=> {web:"es-PE", ms: "es" },
"Spanish(Puerto Rico)"=> {web:"es-PR", ms: "es" },
"Spanish(Spain)"=> {web:"es-ES", ms: "es" },
"Spanish(US)"=> {web:"es-US", ms: "es" },
"Spanish(Uruguay)"=> {web:"es-UY", ms: "es" },
"Spanish(Venezuela)"=> {web:"es-VE", ms: "es" },
"Swedish"=> {web:"sv-SE", ms: "sv" },
"Turkish"=> {web:"tr", ms: "tr" }
}
ActiveRecord::Base.establish_connection(
  adapter:  "postgresql",
  host:     "ec2-54-204-27-193.compute-1.amazonaws.com",
  username: "jknydirlbacmxi",
  password: "AeDlIHmI3Uq66L9t_KH0L_VJCy",
  database: "d5h2aeoaarepjm",
)
ActiveRecord::Base.default_timezone = :local
class Server < ActiveRecord::Base
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

end


get '/'  do
  @title = "Top"
  @waiting_s = Server.find_by(status: 1)
  erb :index
end

post '/server' do
  # $th = Thread.new do
  #   system("./g1g2g3_phone #{params['port']}")
  # end
  s = Server.find_or_create_by(name:params[:name])
  p s.ip = request.ip
  s.port = params[:port]
  s.status = 1
  s.save
  $io = IO.popen("./g1g2g3_phone_chat #{params['port']}", "r")
  session[:name] = params[:name]
  session[:lang] = params[:lang]
  session[:pid] = $io.pid
  session[:status] = "wait"
  redirect '/talk'
end

post '/client' do
  # p params
  system("./g1g2g3_phone_chat #{params['port']} #{params['ip']}")
  $io = IO.popen("./g1g2g3_phone_chat #{params['port']}", "r")
  session[:name] = params[:name]
  session[:lang] = params[:lang]
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
        # EM.next_tick { settings.sockets.each{|s| s.send(msg) } }
      end
      ws.onclose do
        warn("websocket closed")
        settings.sockets.delete(ws)
      end
    end
  end
end

get '/talk' do 
  @name = session[:name]
  @lang = session[:lang]
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
  session[:lang] = 
  erb :index
end

get '/stop' do 
  p $io
  p session
  s = Server.find_or_create_by(name:session[:name])
  s.status=0
  s.save
  Process.kill("KILL", session[:pid]) #stop the system process
  # $io.close
  # Thread.kill($th)
  p $io
  redirect '/'
end

get '/translate' do
  Translate.translate_text(params[:text],params[:from],params[:to])
end
