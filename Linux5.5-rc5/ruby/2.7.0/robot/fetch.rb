require 'nokogiri'
require 'open-uri'
require_relative 'cfg'

def fetch_url(url)
  begin
    # url = url.sub('https://', 'http://')
    html = URI.open(url, {'User-Agent' => $USER_AGENT}).read
  rescue  => error
    printf("! GET URL ERROR, SLEEP 5 SECONDS AND RETRY.  URL=\"%s\"", url)
    puts(error.inspect)
    sleep(5)
    retry
  end
  # html.force_encoding('gbk')
  html.encode('utf-8', 'gbk', :invalid => :replace, :undef => :replace, :replace => '?')
end

def get_doc(url)
  html = fetch_url(url)
  Nokogiri::HTML.parse(html)
end