require 'date'
require_relative 'user'
require_relative 'cfg'


# {"name"=>"MaLaRabbit",
#   "group"=>"\u7528\u6237",
#   "money_all"=>6320.0,
#   "money_usable"=>6319.9,
#   "login"=>14558,
#   "posts"=>7312,
#   "experience"=>20063,
#   "expression"=>64,
#   "vitality"=>6666,
#   "login_date"=>
#    #<DateTime: 2019-12-29T11:39:58+00:00 ((2458847j,41998s,0n),+0s,2299161j)>,
#   "login_ip"=>"73."}

def db_user_create()
  db_exec("DROP TABLE IF EXISTS users")
  db_exec("
    CREATE TABLE users(
      name TEXT PRIMARY KEY,
      in_group TEXT,
      money_all REAL,
      money_usable REAL,
      nr_login INTEGER,
      nr_posts INTEGER,
      experience INTEGER,
      expression INTEGER,
      vitality INTEGER,
      login_date TEXT,
      login_ip TEXT
    )"
  )  
end   

def db_user_update(user)
  m = user_get(user)
  if !m
    puts("! ERROR, get info for \"#{user}\" failed")
    return false
  end

  # puts m

  command = "INSERT INTO users VALUES(" \
    "'#{m['name']}',"  \
    "'#{m['group']}',"  \
    "#{m['money_all']},"  \
    "#{m['money_usable']},"  \
    "#{m['nr_login']},"  \
    "#{m['nr_posts']},"  \
    "#{m['experience']},"  \
    "#{m['expression']},"  \
    "#{m['vitality']},"  \
    "'#{m['login_date'].to_s}',"  \
    "'#{m['login_ip']}'"  \
  ")"

  db_exec(command)
  true
end

def thread_db_user_update(user, idx, total, nr_invalid)
  success = db_user_update(user)
  printf("  %05d/%05d : \"%s\"\n", idx, total, user)
  nr_invalid[0] += 1 if !success    # should be synced
end

def db_user_refresh_all()
  users = db_exec("SELECT DISTINCT name from records ORDER BY name")
  puts("TOTAL #{users.ntuples} users.")
  
  pool = Thread.pool($THREAD_USER_UPDATE)
  nr_invalid = [ 0 ]
  users.each_with_index { |rec, idx|
    user = rec['name']
    pool.process {
      thread_db_user_update(user, idx, users.ntuples, nr_invalid)
    }
  }
  pool.shutdown

  puts("TOTAL #{users.ntuples} users, #{nr_invalid[0]} INVALID USERS")
end

