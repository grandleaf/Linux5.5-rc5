def crawl_top(max_page)
    ops = Set.new
    pool = Thread.pool($THREAD_PER_PAGE)
    mutex = Mutex.new
  
    0.upto(max_page) { |page_nr|
      pool.process {
        url = "http://www.mitbbs.com/newindex/zd_index.php?page=#{page_nr}"
        doc = get_doc(url)
  
        doc.xpath('//table[@width="949"]').each { |tbl|
          links = tbl.xpath('.//a[@class="a2"]')
          links.each { |link|
            href = link["href"]
            next if !href.include?("article_t")
            gid = href.match(/(\d+)\.html/)[1]
  
            rs = db_exec("SELECT user FROM worksheet WHERE gid=#{gid} AND reid=gid")
            next if !rs[0]
            printf("%8s : %-16s : %s\n", gid, rs[0][0], link.content)
  
            mutex.synchronize {
              ops << rs[0][0]
            }
          }
        }
      }
    }
    pool.shutdown
  
    db_exec("DROP TABLE IF EXISTS TopOps")
    db_exec("
      CREATE TABLE TopOps(
        name TEXT PRIMARY KEY
      )
    ")
  
    ops.each { |op|
      db_exec("INSERT INTO TopOps VALUES('#{op}')")
    }
    puts("TOTAL: #{ops.length} TOP OPS FOUND.")
  end
  
  def robot_top_detect()
    robots = db_exec("SELECT * FROM robots WHERE is_robot=1").map(&:first)
    ops = db_exec("SELECT * FROM TopOps").map(&:first)
  
    count = 0
    ops.each { |op|
      next if robots.include?(op)
      next if $WHITE_LIST.include?(op)
  
      info = db_get_user_info(op)
      next if !info
      next if info["expression"] < 60.0
  
      found = false
  
      if robo_detect_by_user_info(info) >= 0.9
        found = true
        printf("%16s : invalid\n", op)
      else
        rs = db_exec("SELECT * FROM robots WHERE name='#{op}'")
        next if rs.empty?
  
        m = record_to_hash(rs[0])
        ratio = m["original_posts"] / m["replies_to_all"].to_f
        if ratio > 10.0
          found = true
          printf("%16s : %f\n", op, ratio)
        end
      end
  
      next unless found
      db_exec("UPDATE robots SET is_robot=1 WHERE name='#{op}'")
      count += 1
    }
  
    puts("TOTAL: #{count}")
  end