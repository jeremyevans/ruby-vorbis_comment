spec = Gem::Specification.new do |s| 
  s.name = "vorbis_comment"
  s.version = "1.0.1"
  s.author = "Jeremy Evans"
  s.email = "code@jeremyevans.net"
  s.homepage = "http://rubyforge.org/projects/vorbiscomment/"
  s.platform = Gem::Platform::RUBY
  s.summary = "Vorbis Comment Reader/Writer Library"
  s.files = Dir["*"]
  s.require_paths = ["."]
  s.extensions << 'extconf.rb'
  s.test_files = Dir["test/*"]
  s.has_rdoc = true
  s.rdoc_options = %w'--inline-source --line-numbers'
  s.add_dependency('cicphash', [">= 1.0.0"])
  s.rubyforge_project = 'vorbiscomment'
  s.homepage = 'http://vorbiscomment.rubyforge.org'
end
