require 'date'
require_relative 'post'
require_relative 'post_list'
require_relative 'cfg'
require 'thread/pool'

def thread_db_rest_update(board, page, index, post)
  rv = post_get(post)
  printf("    + REST %s_%02d_%02d : Page: %02d\n", board, page, index, post['page'])
  rv.each { |p|
    sqlite_update(p, board)
  }  
end

def db_update(date, board, page)
  post_list = post_list_get(board, page)

  pool = Thread.pool($THREAD_PER_PAGE)

  total = 0
  post_list.each_with_index { |post, index|
    next if post['date_create'] < date
    total += 1

    check = sqlite_check_update(post)

    prefix = '='
    if check == :updated || check == :nonexist
      prefix = '+' if check == :nonexist
      prefix = '^' if check == :updated
    end

    str = sprintf("%s %s_%02d_%02d : %d : %s : %s\n", prefix, board, page, index, post['gid'], post['date_create'].to_s, post['title'])
    if check == :same
      printf("%s", str)
      next
    end

    pool.process {
      printf("%s", str)
      post['page'] = 0
      rv = post_get(post)
      rv.each { |p|
        sqlite_update(p, board)
      }


      # db_exec("REPLACE INTO history VALUES('#{post['id']}', '#{post['signature']}', '#{post['date_create']}')")
      db_exec("
        INSERT INTO history 
        VALUES('#{post['id']}', '#{post['signature']}', '#{post['date_create']}')
        ON CONFLICT (id) DO UPDATE
        SET signature='#{post['signature']}', date='#{post['date_create']}'
      ")

      if post['total_pages'] > 1
        rest_pool = Thread.pool($THREAD_PER_REST_PAGE)
        1.upto(post['total_pages'] - 1) { |pg|
          post_clone = post.clone
          post_clone['page'] = pg
          rest_pool.process {
            thread_db_rest_update(board, page, index, post_clone)
          }
        }    
        rest_pool.shutdown   
      end
    
    }
  }

  pool.shutdown
  total
end

def sqlite_update(p, board)
  db_exec("
    INSERT INTO records 
    VALUES(#{p['reid']},#{p['gid']},'#{p['user']}','#{board}','#{p['date']}')
    ON CONFLICT (reid) DO UPDATE
    SET gid=#{p['gid']}, name='#{p['user']}', board='#{board}', date='#{p['date']}'
  ")
  # db_exec("REPLACE INTO records VALUES(#{p['reid']},#{p['gid']},'#{p['user']}','#{board}','#{p['date']}')")
end

def sqlite_check_update(post)
  rs = db_exec("SELECT signature FROM history WHERE id='#{post['id']}'").map { |x| x['signature'] }

  return :nonexist if rs.empty?
  return :updated if rs[0] != post['signature']
  return :same
end

def thread_db_posts_update(date, board, page, stop)
  return if page >= stop[1]

  nr_updates = db_update(date, board, page)
  if nr_updates == 0
    stop[0] = true
    stop[1] = page
    printf("# \"%s\" Page %d has 0 post newer than %s, break!\n", board, page, date.to_s)
  end
end