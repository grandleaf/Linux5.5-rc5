require 'pp'

def db_replace_cmd(table, m)
    "
    INSERT INTO #{table} (#{m.keys.join(', ')})
    VALUES(#{m.values.map { |e| e.is_a?(String) ? "'#{e}'" : e }.join(', ')})
    ON CONFLICT (#{m.keys.first}) DO UPDATE
    SET #{m.map { |k, v| "#{k}=#{v.is_a?(String) ? "'#{v}'" : v}" }.drop(1).join(', ')}
    "
end