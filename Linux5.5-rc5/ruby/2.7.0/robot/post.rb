require 'nokogiri'
require_relative 'fetch'

def get_total_pages(doc)
  # doc.xpath("//div[@id='spam[500]']").first
  total_pages = 1

  doc.xpath("//a").each { |n|
    href = n['href']
    next if href == nil
    next unless href.include?('article_t1')
    # matches = href.match(/_0_(\d+)/)

    begin
      curr_page = href.match(/_0_(\d+)/)[1].to_i
      total_pages = [ total_pages, curr_page ].max
    rescue
      printf("! ERROR: GET TOTAL PAGES ERROR\n")
    end
  }

  total_pages
end

def get_date(date_str)
  comp = date_str.split
  date_str_tz = comp[0..3].join(' ') + "+0500 #{comp[4]}"
  DateTime.parse(date_str_tz)
end

def post_get(m)
  url = "https://www.mitbbs.com/article_t1/#{m['board']}/#{m['gid']}_0_#{m['page'] + 1}.html"
  doc = get_doc(url)

  m['total_pages'] ||= get_total_pages(doc)
  posts = []

  doc.xpath('//*[@class="jiahui-4"]').each_with_index { |n, idx|
    begin
      # grab reid & gid
      reply_node = n.at_xpath('.//a')
      matches = reply_node['href'].match(/reid=(\d+)&gid=(\d+)/)
      post = {
        'reid' => matches[1].to_i,
        'gid' => matches[2].to_i,
      }

      # grab user name
      content_node = n.at_xpath('../../..//*[@class="jiawenzhang-type"]/p')
      content = content_node.content

      post['user'] = content.match(/发信人.*?\s+(.*?)\s/)[1].strip
      date_str = content.match(/发信站.*?\((.*?)\)/)[1].strip
      post['date'] = get_date(date_str)

      posts << post
    rescue
      puts("! Error Parsing: idx=#{idx}, url=\"#{url}\"""")
    end
  }

  posts
end

