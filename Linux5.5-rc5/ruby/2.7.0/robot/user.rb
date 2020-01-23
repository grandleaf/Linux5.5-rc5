require 'nokogiri'
require_relative 'fetch'

def user_get(user)
  begin
    url = "https://www.mitbbs.com/user_info/#{user}"
    doc = get_doc(url)
    node = doc.xpath('//*[@class="bg_whitexian"]//table')[3]

    login_node = doc.xpath('//*[@class="bg_whitexian"]//table')[4]
    login_node = login_node.at_xpath('.//tr')
    matches = login_node.content.match(/\[(.*?)\].*?\[(.*?)\]/)

    login_date_str = matches[1]
    login_date = DateTime.parse(login_date_str)
    login_ip = matches[2]

    trs = node.xpath('.//tr')
    arr = []
    trs.each { |tr|
      tds = tr.xpath('.//td')
      value = tds[2].content.strip
      value = value[/\[(.*?)\]/m, 1].strip if value.include?(']')
      arr << value
    }

    return {
      'name' => user,
      'group' => arr[0],
      'money_all' => arr[1].to_f,
      'money_usable' => arr[2].to_f,
      'nr_login' => arr[3].to_i,
      'nr_posts' => arr[4].to_i,
      'experience' => arr[5].to_i,
      'expression' => arr[6].to_i,
      'vitality' => arr[7].to_i,
      'login_date' => login_date,
      'login_ip' => login_ip
    }
  rescue
    return nil
  end
end