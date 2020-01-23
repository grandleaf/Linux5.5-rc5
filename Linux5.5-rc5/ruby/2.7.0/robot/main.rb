#!/usr/bin/ruby

require "set"

require_relative "crawl"
require_relative "robo_user_info"
require_relative "robo_detect"
require_relative "db_util"

$stdout.sync = true

# m = empty_hash()
# pp db_replace_cmd('robots', m)
# exit

# db_exec("DROP TABLE IF EXISTS records")
# db_exec("DROP TABLE IF EXISTS history")
# db_init()

# crawl_all_boards()
# crawl_user_info()
# exit

robot_copy_table(100)
robot_init("worksheet")
robot_detect("worksheet")


# table = "worksheet"
# processed = 0
# processed += robo_gen_supp(table)
# processed += robo_fill(table)
# processed += robo_supp_detect()
# processed += robo_op_detect(table)
