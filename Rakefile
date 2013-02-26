require 'rake'
require 'rake/clean'
require 'rake/rdoctask'

CLEAN.include %w"rdoc vorbis_comment_ext.*o vcedit.o Makefile mkmf.log vorbis_comment-*.gem"

Rake::RDocTask.new do |rdoc|
  rdoc.rdoc_dir = "rdoc"
  rdoc.options += ["--quiet", "--line-numbers", "--inline-source"]
  rdoc.main = "vorbis_comment.rb"
  rdoc.title = "ruby-vorbis_comment: Vorbis Comment Reader/Writer Library"
  rdoc.rdoc_files.add ["LICENSE", "vorbis_comment.rb"]
end

desc "Update docs and upload to rubyforge.org"
task :doc_rforge => [:rdoc]
task :doc_rforge do
  sh %{chmod -R g+w rdoc/*}
  sh %{scp -rp rdoc/* rubyforge.org:/var/www/gforge-projects/vorbiscomment}
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
