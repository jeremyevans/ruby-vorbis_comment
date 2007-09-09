spec = Gem::Specification.new do |s| 
  s.name = "vorbis_comment"
  s.version = "1.0.0"
  s.author = "Jeremy Evans"
  s.email = "code@jeremyevans.net"
  s.homepage = "http://rubyforge.org/projects/vorbis_comment/"
  s.platform = Gem::Platform::RUBY
  s.summary = "Vorbis Comment Reader/Writer Library"
  s.files = Dir["*"]
  s.require_paths = ["."]
  s.extensions << 'extconf.rb'
  s.autorequire = "vorbis_comment"
  s.test_files = Dir["test/*"]
  s.has_rdoc = true
  s.rdoc_options = %w'--inline-source --line-numbers'
  s.rubyforge_project = 'vorbis_comment'
end

