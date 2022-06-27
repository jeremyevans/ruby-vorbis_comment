spec = Gem::Specification.new do |s| 
  s.name = "vorbis_comment"
  s.version = "1.0.3"
  s.author = "Jeremy Evans"
  s.email = "code@jeremyevans.net"
  s.homepage = "http://ruby-vorbiscomment.jeremyevans.net"
  s.platform = Gem::Platform::RUBY
  s.summary = "Vorbis Comment Reader/Writer Library"
  s.files = %w"LICENSE README.rdoc extconf.rb vcedit.c vcedit.h vorbis_comment.rb vorbis_comment_ext.c"
  s.require_paths = ["."]
  s.extensions << 'extconf.rb'
  s.required_ruby_version = ">= 1.9.2"
  s.rdoc_options = %w'--inline-source --line-numbers'
  s.add_dependency('cicphash', [">= 1.0.0"])
  s.licenses = ["MIT", "LGPL-2.0-or-later"]
end
