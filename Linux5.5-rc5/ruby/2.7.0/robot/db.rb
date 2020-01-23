require "pg"

$MUTEX = Mutex.new
$DB = PG.connect(:dbname => "robot.db", :user => $DB_USER)

def db_exec(command)
  rs = nil
  $MUTEX.synchronize {
    # puts command
    rs = $DB.exec(command)
  }
  rs
end

def db_init()
  begin
    db = PG.connect(:dbname => "robot.db", :user => $DB_USER)

    db_exec("
      CREATE TABLE IF NOT EXISTS records(
        reid INT PRIMARY KEY, 
        gid INT, 
        name TEXT, 
        board TEXT, 
        date TEXT
      )")

    db_exec("
      CREATE TABLE IF NOT EXISTS history(
        id TEXT PRIMARY KEY,
        signature TEXT,
        date TEXT
      )")

    return db
  rescue SPG::Error => e
    puts e.message
    exit
  ensure
    # db.close if db
  end
end
