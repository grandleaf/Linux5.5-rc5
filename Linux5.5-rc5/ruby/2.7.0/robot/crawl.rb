require 'date'
require_relative 'db'
require_relative 'db_board'
require_relative 'user'
require_relative 'db_user'
require_relative 'cfg'
require_relative 'all_boards'

def crawl_records(board:, days: 10, max_page: 100)
  date = Date.today - days

  pool = Thread.pool($THREAD_PER_PAGE)
  stop = [ false,  Float::INFINITY ]
  0.upto(max_page) { |page|
    break if stop[0]
    pool.process {
      thread_db_posts_update(date, board, page, stop)
    }
  }

  pool.shutdown
end


def crawl_all_boards
  time_starting = Time.now
  $ALL_BOARDS.sort.each { |board|
    starting = Time.now
    puts("======================================== STARTING CRAWLLING BOARD \"#{board}\" ========================================")
    crawl_records(board: board, days: $CRAWL_DAYS, max_page: 8888)

    ending = Time.now
    puts("======================================== ENDING CRAWLLING BOARD \"#{board}\", TIME ELAPSED: #{ending - starting} SECONDS ========================================")

  }

  puts("======================================== DONE, TIME ELAPSED: #{(Time.now - time_starting) / 60} MINUTES ========================================")
end

def crawl_user_info()
  db_user_create()
  db_user_refresh_all()
end
