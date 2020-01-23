require_relative "db"
require_relative "cfg"

def user_list_get(db)
  rs = db_exec("SELECT name FROM users")
  rs.map(&:first)
end

def db_get_user_info(user)
  rs = db_exec("SELECT * FROM users WHERE name='#{user}'")
  return nil if rs.ntuples == 0

  c = rs[0]

  info = {
    "name" => c["name"],
    "in_group" => c["in_group"],
    "money_all" => c["money_all"].to_f,
    "money_usable" => c["money_usable"].to_f,
    "nr_login" => c["nr_login"].to_i,
    "nr_posts" => c["nr_posts"].to_i,
    "experience" => c["experience"].to_i,
    "expression" => c["expression"].to_i,
    "vitality" => c["vitality"].to_i,
    "login_date" => DateTime.parse(c["login_date"]),
    "login_ip" => c["login_ip"],
  }
end

def robo_detect_all_by_user_info()
  user_list = user_list_get()

  count = 0
  user_list.each { |name|
    info = db_get_user_info(name)
    weight = robo_detect_by_user_info(info)
    if weight > 0.8
      count += 1
      # puts("#{name} #{info['expression']}")
      puts name
    end
  }
  puts("TOTAL #{count} ROBOTS FOUND.")
end

def robo_detect_by_user_info(info)
  return 0.0 if !info

  nr_login = info["nr_login"].to_f
  expression = info["expression"].to_f
  nr_posts = info["nr_posts"].to_f
  experience = info["experience"].to_f
  vitality = info["vitality"].to_f

  days = nr_login / (expression / 10.0 - nr_posts / nr_login)
  hours = experience - nr_posts - nr_login / 3.0

  info[:days] = days
  info[:hours] = hours
  info[:hours_per_login] = hours / nr_login

  return 1.0 if nr_login <= 0.1
  return 0.0 if expression <= 30.0
  return 1.0 if info[:days] <= 0.0
  return 0.0 if vitality >= 6600.0
  # return 1.0 if info[:hours_per_login] >= 3.0
  return 0.0
end

