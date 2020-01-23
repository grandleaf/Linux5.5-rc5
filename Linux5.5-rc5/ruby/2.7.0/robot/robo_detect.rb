require_relative 'db'
require_relative 'cfg'
require_relative 'db_util'

def robo_gen_users(table)
  db_exec("DROP TABLE IF EXISTS robo_users")
  db_exec("
      CREATE TABLE robo_users(
        name TEXT PRIMARY KEY
      )
    ")
  db_exec("
      INSERT INTO robo_users
      SELECT DISTINCT(name) FROM #{table}
    ")
  rs = db_exec("SELECT COUNT(*) FROM robo_users")
  puts("* TABLE \"robo_users\" CREATED.  TOTAL #{rs[0]["count"]} USERS.")
end

def robo_gen_seeds(table)
  user_list = db_exec("SELECT name FROM robo_users").map { |x| x["name"] }.sort
  puts("* GENERATING robo_seeds FROM USER LIST, LIST SIZE: #{user_list.length} ...")

  db_exec("DROP TABLE IF EXISTS robo_seeds")
  # db_exec("
  #     CREATE TABLE robo_seeds(
  #       name TEXT PRIMARY KEY,
  #       posts INTEGER,
  #       checked INTEGER
  #     )")

  db_exec("
      CREATE TABLE robo_seeds(
        name TEXT PRIMARY KEY,
        original_posts INTEGER,
        replies_to_all INTEGER,
        replies_to_robots INTEGER,
        replies_to_self INTEGER,
        replies_to_others INTEGER,
        supported_by_all INTEGER,
        supported_by_robots INTEGER,
        is_robot INTEGER,
        checked INTEGER
      )")

  count = 0
  starting = Time.now

  pool = Thread.pool($CPU)
  mutex = Mutex.new

  user_list.each_with_index { |name, index|
    pool.process {
      db = PG.connect(:dbname => "robot.db", :user => $DB_USER)

      nr_total = db.exec("SELECT COUNT(*) FROM #{table} WHERE name='#{name}'")[0]["count"]
      gids = db.exec("SELECT gid FROM #{table} WHERE name='#{name}' AND reid=gid").map { |x| x["gid"] }
      original_posts = gids.length

      reply_gids = db.exec("SELECT gid FROM #{table} WHERE name='#{name}' AND reid!=gid").map { |x| x["gid"] }
      replies_to_all = reply_gids.length

      replies_to_self = 0
      reply_gids.each { |r|
        replies_to_self += 1 if gids.include?(r)
      }
      replies_to_others = replies_to_all - replies_to_self

      found = false
      if original_posts >= $ROBOT_DETECT_OP_ONLY_POST_THRESHOLD && original_posts / replies_to_all.to_f > 10.0
        found = true
      else
        info = db_get_user_info(name)
        if robo_detect_by_user_info(info) >= 0.9
          found = true
          printf(" X  invalid info for name=\"%s\", nr_login=%d, days=%f, expression=%f\n",
                 name, info["nr_login"], info[:days], info["expression"])
        end
      end

      # if original_posts >= $ROBOT_DETECT_OP_ONLY_POST_THRESHOLD && original_posts / replies_to_all.to_f > 10.0
      if found
        if (name == "Dreamer" || $WHITE_LIST.include?(name))
          printf(" S  %4d/%d : %20s : op=%-4d : replies=%-4d : self=%-4d : others=%-4d\n",
                 index, user_list.length - 1, name, original_posts, replies_to_all, replies_to_self, replies_to_others)
          db.close if db
          return
        end

        printf(" F  %4d/%d : %20s : op=%-4d : replies=%-4d : self=%-4d : others=%-4d\n", index, user_list.length - 1,
               name, original_posts, replies_to_all, replies_to_self, replies_to_others)

        mutex.synchronize {
          count += 1
        }

        db.exec("INSERT INTO robo_seeds VALUES('#{name}', #{original_posts}, #{replies_to_all}, -1, #{replies_to_self}, #{replies_to_others}, 0, 0, 1, 0)")
      else
        printf("    %4d/%d : %20s : op=%-4d : replies=%-4d : self=%-4d : others=%-4d\n", index, user_list.length - 1, name, original_posts,
               replies_to_all, replies_to_self, replies_to_others)
      end

      db.close if db
    }
  }

  pool.shutdown

  ending = Time.now
  puts("# TOTAL #{count} ROBOTS SEEDS FOUND.  TIME ELAPSED: #{(ending - starting) / 60} MINUTES.")
end

def robo_init_db()
  # create new table
  db_exec("DROP TABLE IF EXISTS robots")
  db_exec("
    CREATE TABLE robots(
      name TEXT PRIMARY KEY,
      original_posts INTEGER,
      replies_to_all INTEGER,
      replies_to_robots INTEGER,
      replies_to_self INTEGER,
      replies_to_others INTEGER,
      supported_by_all INTEGER,
      supported_by_robots INTEGER,      
      is_robot INTEGER,
      checked INTEGER
    )")

  # copy all from robo_seeds
  db_exec("INSERT INTO robots SELECT * FROM robo_seeds")

  # db_exec("SELECT * FROM robo_seeds").each { |r|
  #   db_exec("INSERT INTO robots VALUES('#{r[0]}', #{r[1]}, #{r[0]}, #{r[0]}, #{r[0]}, #{r[0]}, #{r[0]}, #{r[0]}")
  # }
end

def record_to_hash(m)
  {
    "name" => m["name"],
    "original_posts" => m["original_posts"].to_i,
    "replies_to_all" => m["replies_to_all"].to_i,
    "replies_to_robots" => m["replies_to_robots"].to_i,
    "replies_to_self" => m["replies_to_self"].to_i,
    "replies_to_others" => m["replies_to_others"].to_i,
    "supported_by_all" => m["supported_by_all"].to_i,
    "supported_by_robots" => m["supported_by_robots"].to_i,
    "is_robot" => m["is_robot"].to_i,
    "checked" => m["checked"].to_i,
  }
end

def empty_hash()
  {
    "name" => "N/A",
    "original_posts" => 0,
    "replies_to_all" => 0,
    "replies_to_robots" => 0,
    "replies_to_self" => 0,
    "replies_to_others" => 0,
    "supported_by_all" => 0,
    "supported_by_robots" => 0,
    "replies_to_robots_curr" => 0,
    "is_robot" => 0,
    "checked" => 0,
  }

  # record_to_hash(["N/A", 0, 0, 0, 0, 0, 0, 0])
end

# def hash_to_record(m)
#   "'#{m["name"]}', #{m["original_posts"]}, #{m["replies_to_all"]}, #{m["replies_to_robots"]}, " \
#   "#{m["replies_to_self"]}, #{m["replies_to_others"]}, #{m["is_robot"]}, #{m["checked"]}"
# end

def replace_record(m)
  mm = m.clone
  mm.delete('replies_to_robots_curr')
  db_exec(db_replace_cmd('robots', mm))
end

def robo_gen_supp(table)
  starting = Time.now

  supp = {} # todo load
  rs = db_exec("SELECT * FROM robots WHERE is_robot=0")     #.map { |x| x["name"] }
  rs.each { |r|
    m = record_to_hash(r)
    supp[m["name"]] = m
    m["replies_to_robots_curr"] = 0
  }

  seeds = db_exec("SELECT name FROM robots WHERE checked=0 AND is_robot=1").map { |x| x["name"] }.sort
  puts("+ START SEARCHING SUPPORTERS, TOTAL: #{seeds.length}")

  pool = Thread.pool($CPU)
  mutex = Mutex.new

  seeds.each_with_index { |seed, idx|
    pool.process {
      begin
        db = PG.connect(:dbname => "robot.db", :user => $DB_USER)
        printf("+ searching supporter :    %4d/%d : \"%s\"\n", idx + 1, seeds.length, seed)

        # get the gid from the seed (main post)
        gids = db.exec("SELECT gid FROM #{table} WHERE name='#{seed}' AND gid=reid").map { |x| x["gid"].to_i }

        # replies name of each main post
        gids.each { |gid|
          reps = db.exec("SELECT name FROM #{table} WHERE gid=#{gid} AND name!='#{seed}'").map { |x| x["name"] }
          mutex.synchronize {
            reps.each { |rep|
              if !supp[rep]
                supp[rep] = m = empty_hash
                m["name"] = rep
              end

              supp[rep]["replies_to_robots"] += 1
              supp[rep]["replies_to_robots_curr"] += 1
            }
          }
        }

        # this seed has been checked
        db.exec("UPDATE robots SET checked=1 WHERE name='#{seed}'")
        db.close if db
        printf("- searching supporter :    %4d/%d : \"%s\"\n", idx + 1, seeds.length, seed)
      rescue => error
        puts(error.inspect)
      end
    }
  }

  pool.shutdown
  printf("* Searching Done, Updating ...\n")

  count = 0
  robots_array = db_exec("SELECT name FROM robots WHERE is_robot=1").map { |x| x["name"] }
  robots = robots_array.to_set
  supp.each { |name, m|
    next if supp[name]["replies_to_robots_curr"] == 0
    next if robots.include?(name)
    next if $WHITE_LIST.include?(name)

    count += 1
    replace_record(m)

    # db_exec("REPLACE INTO robots VALUES('#{name}', #{m[:replies_to_robots]}, #{m[:replies_to_all]}, #{m[:original_posts]}, #{m[:is_robot]}, #{m[:checked]})")
  }

  ending = Time.now
  puts("- DONE SEARCHING SUPPORTERS, FOUND: #{count}.  TIME ELAPSED: #{(ending - starting) / 60} MINUTES.")
  count
end

def robo_fill(table)
  users = db_exec("SELECT name FROM robots WHERE checked=0 AND is_robot=0 AND replies_to_all=0 AND original_posts=0").map { |x| x["name"] }
  puts("+ START ROBOTS FILLING, TOTAL: #{users.length}")
  starting = Time.now

  pool = Thread.pool($CPU)
  # mutex = Mutex.new
  # count = 0

  users.each_with_index { |name, idx|
    pool.process {
      begin
        db = PG.connect(:dbname => "robot.db", :user => $DB_USER)
        printf("fill    %4d/%d : \"%s\"\n", idx + 1, users.length, name)

        gids = db.exec("SELECT gid FROM #{table} WHERE name='#{name}' AND reid=gid").map { |x| x["gid"] }
        original_posts = gids.length

        reply_gids = db.exec("SELECT gid FROM #{table} WHERE name='#{name}' AND reid!=gid").map { |x| x["gid"] }
        replies_to_all = reply_gids.length

        replies_to_self = 0
        reply_gids.each { |r|
          replies_to_self += 1 if gids.include?(r)
        }
        replies_to_others = replies_to_all - replies_to_self

        supported_by_all = 0
        gids.each { |gid|
          rs = db.exec("SELECT count(*) FROM #{table} WHERE reid=#{gid}")
          supported_by_all += rs[0]["count"].to_i
          # supported_by_all = db.exec("SELECT count(*) FORM #{table} WHERE reid==#{gid}")
        }
        # puts supported_by_all
        # exit

        # "supported_by_all" => 0,
        # "supported_by_robots" => 0,

        db.exec("
              UPDATE robots
              SET 
                original_posts=#{original_posts}, 
                replies_to_all=#{replies_to_all}, 
                replies_to_self=#{replies_to_self}, 
                replies_to_others=#{replies_to_others},
                supported_by_all=#{supported_by_all}
              WHERE name='#{name}';
            ")

        # mutex.synchronize {
        #   count += 1
        # }

        db.close if db
      rescue => error
        puts(error.inspect)
        exit
      end
    }
  }

  pool.shutdown

  ending = Time.now
  puts("- DONE ROBOTS FILLING, TOTAL: #{users.length}.  TIME ELAPSED: #{(ending - starting) / 60} MINUTES.")
  # count # should equal to users.length
  users.length
end

def robo_supp_detect()
  rs = db_exec("SELECT * FROM robots WHERE is_robot=0")
  count = 0
  rs.each { |row|
    # m = record_to_hash(row)
    m = row
    replies_to_robots = m["replies_to_robots"].to_f
    replies_to_all = m["replies_to_all"].to_f

    ratio = replies_to_robots / replies_to_all
    # if replies_to_robots >= 2 && ratio > $ROBOT_DETECT_REPLIES_ROBOT_ALL_RATIO # criteria

    if replies_to_robots >= $ROBOT_DETECT_REPLIES_TO_ROBOT_THRESHOLD && ratio > $ROBOT_DETECT_REPLIES_ROBOT_ALL_RATIO # criteria
      # puts("    #{name} #{ratio}")
      printf("%20s: replies_to_robots=%4d, ration=%f\n", m["name"], replies_to_robots, ratio)
      db_exec("UPDATE robots SET is_robot=1 WHERE name='#{m["name"]}'")
      count += 1
    end
  }

  puts("* ROBOTS SUPPORTERS FOUND.  TOTAL: #{count}.")
  count
end

def robo_op_detect(table)
  supp = db_exec("SELECT name FROM robots WHERE is_robot=1 AND checked=0").map { |x| x["name"] }.sort
  puts("+ STARTING OP DETECTING, TOTAL: #{supp.length}")
  starting = Time.now

  ops = {}
  pool = Thread.pool($CPU)
  mutex = Mutex.new

  supp.each_with_index { |name, idx|
    pool.process {
      begin
        db = PG.connect(:dbname => "robot.db", :user => $DB_USER)
        printf("  searching robot op:    %4d/%d : \"%s\"\n", idx + 1, supp.length, name)

        # new robots replied to these gids
        gids = db.exec("SELECT gid FROM #{table} WHERE name='#{name}' AND gid!=reid").map { |x| x["gid"] }

        gids.each { |gid|
          op_list = db.exec("SELECT name FROM #{table} WHERE gid=#{gid} AND gid=reid").map { |x| x["name"] }
          next if op_list.empty?

          op = op_list[0]
          mutex.synchronize {
            if !ops[op]
              ops[op] = 1
            else
              ops[op] += 1
            end
          }
        }
        db.close if db
      rescue => error
        puts(error.inspect)
        exit
      end
    }
  }

  pool.shutdown

  robots = db_exec("SELECT * FROM robots WHERE is_robot=1").map { |x| x["name"] }
  found = 0
  ops.each { |name, count|
    rs = db_exec("SELECT * FROM robots WHERE name='#{name}'")

    m = rs.ntuples == 1 ? record_to_hash(rs[0]) : empty_hash()
    m["name"] = name
    m["supported_by_robots"] += count
    replace_record(m)

    # puts("#{name}, #{count}")
    next if m["supported_by_robots"] <= $ROBOT_DETECT_SUPPORTED_BY_ROBOT      # criteria

    # puts("ROBOT INCLUDE? #{name}") if robots.include?(name)
    next if robots.include?(name)
    next if $WHITE_LIST.include?(name)

    # # puts("gagaga")
    # pp m

    # db_exec("UPDATE robots SET is_robot=1 WHERE name='#{name}'")
    found += 1
    printf("Found %d, name=\"%s\", count=%d\n", found, name, m["supported_by_robots"])
  }

  ending = Time.now
  puts("- DONE OP DETECTING, FOUND: #{found}.  TIME ELAPSED: #{(ending - starting) / 60} MINUTES.")
  found
end

def robo_output()
  robots = db_exec("SELECT name FROM robots WHERE is_robot=1 order by name").map { |x| x["name"] }
  File.open("robot_set.txt", "w") { |file| file.write(robots.join("\n")) }
  puts("FILE \"robot_set.txt\" WRITTEN, TOTAL #{robots.length} ROBOTS!")
end

def robot_copy_table(days)
  date = DateTime.now - days
  db_exec("DROP TABLE IF EXISTS worksheet")
  db_exec("CREATE TABLE worksheet AS SELECT * FROM records WHERE date>='#{date}'")
end

def robot_init(table)
  robo_gen_users(table)
  robo_gen_seeds(table)
  robo_init_db()
end

def robot_detect(table)
  1.upto($ROBOT_DETECTING_PASS) { |pass|
    printf("\n\n############################################## PASS #{pass}/#{$ROBOT_DETECTING_PASS} ##############################################\n")

    processed = 0
    processed += robo_gen_supp(table)
    processed += robo_fill(table)
    processed += robo_supp_detect()
    processed += robo_op_detect(table)

    if processed == 0
      puts("* PROCESSED = 0, BREAK.")
    end
  }

  robo_output()
end
