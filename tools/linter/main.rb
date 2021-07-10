#!/usr/bin/env ruby
# must be run in the root directory of the repo
# currently it only checks if code in src/kernel/ uses anything arch-dependent

$return = 0

def warn(msg)
  STDERR.puts "[LINTER] #{msg}"
  $return = 1
end

whitelist = ["arch/generic.h"]

Dir["src/kernel/**.?"]
  .each do |path|
    File.read(path)
      .lines
      .map(&:strip)                          # strip whitespace
      .filter{|line| line[0] == '#'}         # find preprocessor directives
      .map{|line| line[1..].strip}           # get rid of the #, strip again
      .filter{|l| l.start_with? "include"}   # find includes
      .map{|l| /([<"](.+)[">])/.match(l)[2]} # get the name of the included file
      .filter{|l| l.start_with? "arch/"}     # find files in arch/
      .filter{|l| not whitelist.include? l}  # filter out whitelisted headers
      .each{|inc| warn "#{path} includes #{inc}"}
  end

exit $return
