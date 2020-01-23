#!/usr/bin/ruby

require 'fileutils'
require 'pp'
require 'ffi'

WORK_DIR = '/home/grandleaf/tmpfs/raid/out/bin'
CHUNK_SIZE = 128 << 10
CI_TRACE_FRAME_HEAD_SIZE = 16

# sync with the C code
CI_STR_PTR_SIZE					= 4

CI_TRACE_TYPE_CONST_STR 		= 0
CI_TRACE_TYPE_U8				= 1
CI_TRACE_TYPE_U16				= 2
CI_TRACE_TYPE_U32				= 3
CI_TRACE_TYPE_U64				= 4
CI_TRACE_NR_FIXED_TYPE			= 5
CI_TRACE_MAX_NON_CONST_STR_LEN 	= 0xFF - CI_TRACE_NR_FIXED_TYPE

type_to_size = [CI_STR_PTR_SIZE, 1, 2, 4, 8 ]
$ffi_type = [':string', ':uint8', ':uint16', ':uint32', ':uint64']



module LIBC
  extend FFI::Library
  ffi_lib "/lib/x86_64-linux-gnu/libc.so.6"
  
  attach_function 'printf', 	[:string, :varargs], 			:int
  attach_function 'sprintf', 	[:pointer, :string, :varargs], 	:int
  attach_function 'malloc', 	[:size_t],						:pointer
end

class Integer
  def to_b?
    !self.zero?
  end
end

# hack, change in the future
def get_string(str)
	str[3 .. str.length - 1]
end

def format_string_handling(str)
	(str.chomp('') + '\n').gsub(/(%\-?\d*)ll([xX])/, '\1\2')   
end

def format_string(str, idx)
	str.chomp!
	str += '\n' if idx == 0
	str = '"' + str + '"'
	str = $ffi_type[CI_TRACE_TYPE_CONST_STR] + ', ' + str if idx > 0
	str
end	


sprintf_buf = LIBC.malloc(64 * 1024)		# should be big enough

# code start here
hash = {}
string_txt = `strings -tx #{File.join(WORK_DIR, 'raid.sim')}`
for line in string_txt.split("\n") do
	duplet = line.strip.split(' ', 2)
	hash["%08X" % duplet[0].hex] = duplet[1]
end

#pp hash

bytes = File.binread(File.join(WORK_DIR, 'trace.bin'))
all_outputs = []
max_filename_len = 0

idx = 0
while idx < bytes.length
	if idx % CHUNK_SIZE == 0
		idx += CI_TRACE_FRAME_HEAD_SIZE
		next
	end

	num_args = bytes[idx].ord
	
	if num_args == 0
		skip = CHUNK_SIZE - idx % CHUNK_SIZE
		idx += skip
		next
	end
   
	args_type = bytes[idx + 1, num_args].unpack('C*') 
	idx += num_args + 1
	
#	pp "#{num_args}, #{args_type}"

	cmd = ''
	arg_idx = -1
	for type in args_type
		arg_idx += 1
		cmd += sprintf(", ") if arg_idx != 0

		# non-const string
		if type >= CI_TRACE_NR_FIXED_TYPE
			size = type - CI_TRACE_NR_FIXED_TYPE
			value = bytes[idx, size]
			value = "" if value == nil
			value = format_string(value, arg_idx)
			idx += size
			cmd += value
			next
		end
		
		size = type_to_size[type]
		value = bytes[idx, size].reverse.unpack('H*')[0]
		idx += size
		if type != CI_TRACE_TYPE_CONST_STR
			cmd += $ffi_type[type] + ', 0x' + value
		else
			value = "%08X" % get_string(value).hex
			value = hash[value]
			if value
				value = value[1..-5]
				value.slice!('\\n')
			else
				value = ''
			end
			value = format_string(value, arg_idx)
			cmd += value
		end
	end
	
	# meta
	timestamp = bytes[idx, 8].reverse.unpack('H*')[0]	# string
	idx += 8
	
	filename = hash["%08X" % get_string(bytes[idx, 4].reverse.unpack('H*')[0]).hex]
	idx += 4
	
	line = bytes[idx, 2].reverse.unpack('H*')[0]
	idx += 2

	u16_meta = bytes[idx, 2].reverse.unpack('H*')[0]
	idx += 2
	
	max_filename_len = [filename.length, max_filename_len].max
	meta = [timestamp.hex, filename, line.hex, u16_meta.hex]

	eval_cmd = "LIBC.sprintf(sprintf_buf, #{cmd})"
	eval(eval_cmd)
	all_outputs << [meta, sprintf_buf.read_string]
end

meta_fmt = "%016X    %04X    %-#{max_filename_len}s    %05d    "

all_outputs.sort! { |x, y|
	x[0][0] <=> y[0][0]
}

all_outputs.each { |x|
	meta = x[0]
	str  = x[1]
	printf(meta_fmt, meta[0], meta[3], meta[1], meta[2])
	printf("%s", str)
}












