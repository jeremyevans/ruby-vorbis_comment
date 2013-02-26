spec = Gem::Specification.new do |s| 
  s.name = "vorbis_comment"
  s.version = "1.0.2"
  s.author = "Jeremy Evans"
  s.email = "code@jeremyevans.net"
  s.homepage = "http://rubyforge.org/projects/vorbiscomment/"
  s.platform = Gem::Platform::RUBY
  s.summary = "Vorbis Comment Reader/Writer Library"
  s.files = %w"LICENSE Rakefile extconf.rb test test/manyfields.ogg test/corrupt.ogg test/lt8k.ogg test/blank.ogg test/test_vorbis_comment.rb test/title.ogg test/empty_key.ogg vcedit.c vcedit.h vorbis_comment.rb vorbis_comment_ext.c"
  s.require_paths = ["."]
  s.extensions << 'extconf.rb'
  s.has_rdoc = true
  s.rdoc_options = %w'--inline-source --line-numbers'
  s.add_dependency('cicphash', [">= 1.0.0"])
  s.rubyforge_project = 'vorbiscomment'
  s.homepage = 'http://vorbiscomment.rubyforge.org'
end
