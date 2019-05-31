#!/usr/bin/env ruby
require 'rubygems'
require 'vorbis_comment'
require 'fileutils'
ENV['MT_NO_PLUGINS'] = '1' # Work around stupid autoloading of plugins
require 'minitest/autorun'

begin
  VorbisComment.new(File.join(File.dirname(__FILE__), 'lt8k.ogg')).fields
rescue VorbisComment::InvalidDataError
  puts('The linked version of libvorbis has problems with files < 8K in size')
end

class VorbisCommentTest < Minitest::Test
  def vc(filename)
    VorbisComment.new(File.join(File.dirname(__FILE__), filename))
  end
  
  def work_with_copy(filename, &block)
    new_name = File.join(File.dirname(__FILE__), "copy-#{filename}")
    FileUtils.cp(File.join(File.dirname(__FILE__), filename), new_name)
    begin
      result = yield new_name
    ensure
      File.delete(new_name)
    end
    result
  end
  
  def test_initialize
    assert_equal File.join(File.dirname(__FILE__), 'title.ogg'), vc('title.ogg').filename
    # Nonexistent files don't raise an error until they are accessed for the fields
    assert_equal 'nonexistant.ogg', VorbisComment.new('nonexistant.ogg').filename
    # Corrupt files don't raise an error until they are parsed for the fields
    assert_equal File.join(File.dirname(__FILE__), 'corrupt.ogg'), vc('corrupt.ogg').filename
  end
  
  def test_exists?
    assert_equal true, vc('title.ogg').exists?
    assert_equal true, vc('manyfields.ogg').exists?
    # Blank tags still exist
    assert_equal true, vc('blank.ogg').exists?
    # Corrupt tags are considered not to exist
    assert_equal false, vc('corrupt.ogg').exists?
    assert_equal false, vc('empty_key.ogg').exists?
    # Files that aren't ogg files will be treated similarly
    assert_equal false, vc('test_vorbis_comment.rb').exists?
    # But nonexistant files raise an OpenError
    assert_raises(VorbisComment::OpenError){vc('nonexistant.ogg').exists?}
  end
  
  def test_fields
    assert_equal Hash[], vc('blank.ogg').fields
    assert_equal Hash['title'=>['Silence']], vc('title.ogg').fields
    assert_equal Hash[{"ARTIST"=>["Test Artist 1", "Test Artist 2"], "TrackNumber"=>["1"], "Album"=>["Test Album"], "Title"=>["Test Title"], "Date"=>["1999-12-31"], "comment"=>[""]}], vc('manyfields.ogg').fields
    assert_raises(VorbisComment::InvalidDataError){vc('empty_key.ogg').fields}
    assert_raises(VorbisComment::InvalidDataError){vc('corrupt.ogg').fields}
    assert_raises(VorbisComment::OpenError){vc('nonexistant.ogg').fields}
  end
  
  def test_remove!
    assert_equal Hash[], work_with_copy('blank.ogg'){|filename| VorbisComment.new(filename).remove!}
    assert_equal Hash[], work_with_copy('title.ogg'){|filename| VorbisComment.new(filename).remove!}
    assert_equal Hash[], work_with_copy('manyfields.ogg'){|filename| VorbisComment.new(filename).remove!}
    assert_raises(VorbisComment::InvalidDataError){work_with_copy('empty_key.ogg'){|filename| VorbisComment.new(filename).remove!}}
    assert_raises(VorbisComment::InvalidDataError){work_with_copy('corrupt.ogg'){|filename| VorbisComment.new(filename).remove!}}
    assert_raises(VorbisComment::OpenError){VorbisComment.new('nonexistant.ogg').remove!}
  end
  
  def test_pretty_print
    assert_equal "", vc('blank.ogg').pretty_print
    assert_equal "title: Silence", vc('title.ogg').pretty_print
    assert_equal "ARTIST: Test Artist 1, Test Artist 2\nAlbum: Test Album\nDate: 1999-12-31\nTitle: Test Title\nTrackNumber: 1\ncomment: ", vc('manyfields.ogg').pretty_print
    assert_equal "CORRUPT TAG!", vc('empty_key.ogg').pretty_print
    assert_equal "CORRUPT TAG!", vc('corrupt.ogg').pretty_print
    assert_equal "CORRUPT TAG!", vc('test_vorbis_comment.rb').pretty_print
  end
  
  def test_update
    f = {'Blah'=>['Blah']}
    assert_raises(VorbisComment::InvalidDataError){work_with_copy('empty_key.ogg'){|filename| VorbisComment.new(filename).update{|fields| fields.merge!(f)}}}
    assert_raises(VorbisComment::InvalidDataError){work_with_copy('corrupt.ogg'){|filename| VorbisComment.new(filename).update{|fields| fields.merge!(f)}}}
    assert_raises(VorbisComment::OpenError){VorbisComment.new('nonexistant.ogg').update{|fields| fields.merge!(f)}}
    
    g = {'x'=>'y', 'y'=>:z, :z=>[:A], :zz=>['A', :b, 'c']}
    h = {'x'=>['y'], 'y'=>['z'], 'z'=>['A'], 'zz'=>['A', 'b', 'c']}
    
    %w'blank.ogg title.ogg manyfields.ogg'.each do |fname|
      work_with_copy(fname) do |filename|
        v = VorbisComment.new(filename)
        fx = v.fields.merge(f)
        assert_equal fx, v.update{|fields| fields.merge!(f)}
        assert_equal fx, v.fields
        v = VorbisComment.new(filename)
        assert_equal fx, v.fields
        gx = v.fields.merge(g)
        assert_equal gx, v.update{|fields| fields.merge!(g)}
        v = VorbisComment.new(filename)
        hx = v.fields.merge(h)
        assert_equal hx, VorbisComment.new(filename).fields
      end
    end
  end
    
  def test_bad_keys_and_values
    work_with_copy('blank.ogg') do |filename|
      v = VorbisComment.new(filename)
      # Test for bad keys
      (("\x00".."\x1f").to_a + ['='] + ("\x7e".."\xff").to_a).each do |c|
        assert_raises(VorbisComment::InvalidCommentError){v.update{|fields| fields[c] = 'Blah'}}
      end
      # Test for bad vales (Invalid UTF8)
      assert_raises(VorbisComment::InvalidCommentError){v.update{|fields| fields['Blah'] = "\x81\x81"}}
    end
  end
    
  def test_add_to_fields_and_normalize_fields
    work_with_copy('blank.ogg') do |filename|
      v = VorbisComment.new(filename)
      assert_equal Hash[], v.fields
      assert_equal [], v.send(:normalize_fields)
      v.send(:add_to_fields, 'A', 'b')
      assert_equal CICPHash['A'=>['b']], v.fields
      assert_equal [['A', 'b']], v.send(:normalize_fields)
      v.send(:add_to_fields, :a, 'C')
      assert_equal CICPHash['A'=>['b','C']], v.fields
      assert_equal [['A', 'C'], ['A', 'b']], v.send(:normalize_fields)
      v.send(:add_to_fields, 12, 42)
      assert_equal CICPHash['A'=>['b','C'], 12=>[42]], v.fields
      assert_equal [['12', '42'], ['A', 'C'], ['A', 'b']], v.send(:normalize_fields)
      v.update{}
      assert_equal CICPHash['A'=>['C','b'], '12'=>['42']], VorbisComment.new(filename).fields
      assert_equal [['12', '42'], ['A', 'C'], ['A', 'b']], v.send(:normalize_fields)
    end
  end
end
