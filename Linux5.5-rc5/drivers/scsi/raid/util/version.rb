#!/usr/bin/ruby

require 'fileutils'

build_no = 0

File.open(File.join(__dir__, 'build_nr.h'), 'r+') { |f|
	build_no = f.readline().to_i + 1
	f.rewind()
	f.puts(build_no)
}

File.open(File.join(__dir__, 'ci_raid_ver.h'), 'w+') do |f|
	f.puts("#define CI_RAID_BUILD_NR     \"#{build_no}\"")
end
