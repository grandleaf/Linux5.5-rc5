# for a board, e.g. Games, fetch page 0, page 1, ....
$THREAD_PER_BOARD = 5       # 5

# for all the gid in a gid_list
$THREAD_PER_PAGE = 20        # 20

# for the same gid, rest of the pages
$THREAD_PER_REST_PAGE = 5

# how many concurrent for grab user info
$THREAD_USER_UPDATE = 100

# crawl date
$CRAWL_DAYS = 2

# user agent
$USER_AGENT = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_2) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.88 Safari/537.36'


# ROBOT DETECTING PASS
$ROBOT_DETECTING_PASS =                             5

#
# ROBOT DETECTING PARAMETERS: PER MONTH
#
$ROBOT_DETECT_OP_ONLY_POST_THRESHOLD =              20      # 20, for detecting seed, post > this value and never reply
$ROBOT_DETECT_REPLIES_TO_ROBOT_THRESHOLD =          15      # 10, reply to robot > this value then check RATIO
$ROBOT_DETECT_SUPPORTED_BY_ROBOT =                  5       # 5, this number of robots support you

# NOT SCALE WITH MONTH
$ROBOT_DETECT_REPLIES_ROBOT_ALL_RATIO =             0.75     # replies_to_robot / replies_to_all > this value means robot

$WHITE_LIST = [ 'tiangeng' ]

$CPU = 32

$DB_USER = "hua"