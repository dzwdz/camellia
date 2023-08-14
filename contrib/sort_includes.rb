#!/usr/bin/env ruby

def is_include str
  str.start_with? '#include'
end

files = ARGV
if files.empty?
  default = "src/**/*.[ch]"
  puts "no arguments passed, defaulting to #{default}"
  files = Dir[default]
end

files.each { |path|
  File.open(path, "r+") do |file|
    lines = file.readlines

    last = nil
    grouped = []
    lines.each do |line|
      if is_include(line) != last
        grouped << [line]
        last = is_include(line)
      else
        grouped[-1] << line
      end
    end

    grouped.map do |group|
      group.sort! if is_include group[0]
    end
    grouped = grouped.flatten

    next if grouped == lines

    puts path
    file.truncate(0)
    file.seek(0)
    file.write grouped.join
  end
}

