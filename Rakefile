require 'rake/clean'

CLEAN.include %w"rdoc vorbis_comment_ext.*o vcedit.o Makefile mkmf.log vorbis_comment-*.gem coverage"

desc "Generate rdoc"
task :rdoc do
  rdoc_dir = "rdoc"
  rdoc_opts = ["--line-numbers", "--inline-source", '--title', 'ruby-vorbis_comment: Vorbis Comment Reader/Writer Library']

  begin
    gem 'hanna'
    rdoc_opts.concat(['-f', 'hanna'])
  rescue Gem::LoadError
  end

  rdoc_opts.concat(['--main', 'README.rdoc', "-o", rdoc_dir] +
    %w"README.rdoc CHANGELOG LICENSE vorbis_comment.rb"
  )

  FileUtils.rm_rf(rdoc_dir)

  require "rdoc"
  RDoc::RDoc.new.document(rdoc_opts)
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
