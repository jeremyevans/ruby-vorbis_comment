require 'rake'
require 'rake/clean'
require 'rake/rdoctask'

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
task :package do
  sh %{gem build vorbis_comment.gemspec}
end
