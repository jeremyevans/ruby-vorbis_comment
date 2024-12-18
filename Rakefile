require 'rake/clean'
require "rdoc/task"

CLEAN.include %w"rdoc vorbis_comment_ext.*o vcedit.o Makefile mkmf.log vorbis_comment-*.gem coverage"

RDoc::Task.new do |rdoc|
  rdoc.rdoc_dir = "rdoc"
  rdoc.options += ["--quiet", "--line-numbers", "--inline-source", '--title', 'ruby-vorbis_comment: Vorbis Comment Reader/Writer Library', '--main', 'README.rdoc']

  begin
    gem 'hanna'
    rdoc.options += ['-f', 'hanna']
  rescue Gem::LoadError
  end

  rdoc.rdoc_files.add %w"README.rdoc LICENSE vorbis_comment.rb"
end

desc "Package ruby-vorbis_comment"
task :package => :clean do
  sh %{#{FileUtils::RUBY} -S gem build vorbis_comment.gemspec}
end

desc "Build extension"
task :build => :clean do
  sh %{#{FileUtils::RUBY} extconf.rb && make}
end

desc "Run tests without building"
task :test do
  sh %{#{FileUtils::RUBY} #{"-w" if RUBY_VERSION >= '3'} #{'-W:strict_unused_block' if RUBY_VERSION >= '3.4'} test/test_vorbis_comment.rb}
end

desc "Run tests"
task :default => [:build, :test]

desc "Run tests with coverage"
task :test_cov => [:build] do
  ENV['COVERAGE'] = '1'
  sh %{#{FileUtils::RUBY} test/test_vorbis_comment.rb}
end
