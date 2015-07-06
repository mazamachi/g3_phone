CLIENT_ID       = 'showmeear_translator'
CLIENT_SECRET   = 'UYCoMgxVyB+MzE2fDqyhYk/BAWK0B0RLHw1nw1HlIyI='
AUTHORIZE_URL   = 'https://datamarket.accesscontrol.windows.net/v2/OAuth2-13'
TRANSLATION_URL = 'http://api.microsofttranslator.com/V2/Http.svc/Translate'
SCOPE           = 'http://api.microsofttranslator.com'
require 'uri'
require 'net/http'
require 'cgi'
require 'json'
require 'rexml/document'
class Translate
  def self.get_access_token
    @access_token = nil
    auth_uri = URI.parse(AUTHORIZE_URL)
    https = Net::HTTP.new(auth_uri.host,443)
    https.use_ssl = true
    query_string= "grant_type=client_credentials&client_id=#{CGI.escape(CLIENT_ID)}&client_secret=#{CGI.escape(CLIENT_SECRET)}&scope=#{CGI.escape(SCOPE)}"
    response=https.post(auth_uri.path, query_string)
    json = JSON.parse(response.body)
    @access_token = json['access_token']
  end

  def self.translate_text(text,from,to)
    self.get_access_token
    trans_uri=URI.parse(TRANSLATION_URL)
    http=Net::HTTP.new(trans_uri.host)
    res = http.get(trans_uri.path+"?from=#{from}&to=#{to}&text=#{URI.escape(text)}",'Authorization' => "Bearer #{@access_token}")
    xml = REXML::Document.new(res.body)
    xml.root.text
  end
end