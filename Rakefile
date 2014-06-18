require 'rake'
require 'rake/clean'

CLEAN.include %w"rdoc vorbis_comment_ext.*o vcedit.o Makefile mkmf.log vorbis_comment-*.gem"

RDOC_OPTS = ["--line-numbers", "--inline-source", '--main', 'README']

begin
  gem 'hanna-nouveau'
  RDOC_OPTS.concat(['-f', 'hanna'])
rescue Gem::LoadError
end

rdoc_task_class = begin
  require "rdoc/task"
  RDoc::Task
rescue LoadError
  begin
    require "rake/rdoctask"
    Rake::RDocTask
  rescue LoadError, StandardError
  end
end

if rdoc_task_class
  rdoc_task_class.new do |rdoc|
    rdoc.rdoc_dir = "rdoc"
    rdoc.options += RDOC_OPTS
    rdoc.main = "vorbis_comment.rb"
    rdoc.title = "ruby-vorbis_comment: Vorbis Comment Reader/Writer Library"
    rdoc.rdoc_files.add ["LICENSE", "vorbis_comment.rb"]
  end
end

desc "Package ruby-vorbis_comment"
task :package => :clean do
  sh %{#{FileUtils::RUBY} -S gem build vorbis_comment.gemspec}
end

desc "Build extension"
task :build => :clean do
  sh %{#{FileUtils::RUBY} extconf.rb && make}
end

desc "Run tests"
task :default do
  sh %{#{FileUtils::RUBY} test/test_vorbis_comment.rb}
end
