require 'nokogiri'
require_relative 'fetch'

def post_list_get(board, page)
  url = "https://www.mitbbs.com/bbsdoc1/#{board}_#{(page * 100 + 1).to_s.rjust(3, '0')}_0.html"
  doc = get_doc(url)

  post_list = []
  doc.xpath('//*[@class="news1"]').each { |n|
    img_node = n.parent.parent.parent.children[1]
    next if img_node.inner_html.include?('istop.gif')

    href = n['href']
    next unless href.include?('article_t')

    title = n.content.strip.delete_prefix('●').strip
    components = href.split('/')
    board = components[2]
    gid = components[3].delete_suffix('.html')

    create_node = n.parent.parent.next.next.next
    reply_node = create_node.next

    op_node = create_node.at_xpath('.//a')
    op = op_node.content.strip
    date_create_node = create_node.at_xpath('.//span')
    date_create = Date.parse(date_create_node.content.strip)
    signature = op + ' ' + date_create.to_s

    lp = nil
    date_reply_str = nil
    lp_node = reply_node.at_xpath('.//a')
    if lp_node 
      lp = lp_node.content.strip
      data_reply_node = reply_node.at_xpath('.//span')
      date_reply_str = data_reply_node.content.strip
      signature = lp + ' ' + date_reply_str
    end

    post = {
      'board' => board,
      'gid' => gid,
      'title' => title,
      'op' => op,
      'date_create' => date_create,
      'lp' => lp,
      'date_reply_str' => date_reply_str,
      'id' => board + '_' + gid.to_s,
      'signature' => signature,
    }

    post_list.push(post)
  }

  post_list
end