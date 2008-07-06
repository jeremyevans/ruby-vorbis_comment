#!/usr/bin/env ruby
# This library implements a Vorbis Comment reader/writer.
# If called from the command line, it prints out the contents of the APEv2 tag 
# for the given filename arguments.
#
# ruby-vorbis_comment is a pure Ruby library for manipulating Vorbis comments.
# It wraps libvorbis and libogg, so it should be completely standards
# compatible. Vorbis comment is the standard tagging format for Ogg Vorbis,
# FLAC, and Speex files.
#
# The library includes a C extension, which may or may not build cleanly on all
# architectures. It is developed and tested on OpenBSD i386 and amd64.  If it
# doesn't work on your machine, please try to get it to work and send in a
# patch.
#
# This library tries to be API compatible with ruby-apetag, a library for
# reading and writing APE tags, the standard tagging format for Musepack and
# Monkey's Audio, which can also be used with MP3s as an alternative to ID3v2.
#
# General Use:
#
#  require 'vorbiscomment'
#  a = VorbisComment.new('file.ogg')
#  a.exists? # if it already has an vorbis comment
#  a.fields # a CICPHash of fields, keys are strings, values are lists of strings
#  a.pretty_print # string suitable for pretty printing
#  a.update{|fields| fields['Artist']='Test Artist'; fields.delete('Year')}
#   # Update the tag with the added/changed/deleted fields
#   # Note that you should do: a.update{|fields| fields.replace('Test'=>'Test')}
#   # and NOT: a.update{|fields| fields = {'Test'=>'Test'}}
#   # You need to update/modify the fields given, not reassign it
#  a.remove! # clear the list of vorbis comments from the file
#
# To run the tests for the library, run test_vorbis_comment.rb.
#
# If you find any bugs, would like additional documentation, or want to submit a
# patch, please use Rubyforge (http://rubyforge.org/projects/vorbis_comment/).
# One known bug is that it doesn't like files less than 8K in size.
#
# The RDoc for the project is available on http://vorbiscomment.rubyforge.org.
#
# The most current source code can be accessed via github
# (http://github.com/jeremyevans/ruby-vorbis_comment/).  Note that 
# the library isn't modified on a regular basis, so it is unlikely to be
# different from the latest release.
#
# The pure ruby part of this library is copyright Jeremy Evans and is released
# under the MIT License.
#
# The C part of this library is copyright Jeremy Evans and Tilman Sauerbeck and
# is licensed under the GNU LGPL.
#
# Copyright (c) 2007 Jeremy Evans
#
# Permission is hereby granted, free of charge, to any person obtaining a copy 
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights 
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
# copies of the Software, and to permit persons to whom the Software is 
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in 
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
# SOFTWARE.

require 'cicphash'

# Contains all of the Vorbis comment fields found in the filename/file given.
class VorbisComment
  attr_reader :filename
  BAD_KEY_RE = /[\x00-\x1f=\x7e-\xff]/
  
  # Set the filename to be used
  def initialize(filename)
    @filename = filename.to_s
  end
  
  # Check for the existance of the a vorbis comment in the file.
  # Because all Vorbis bitstreams should have a comment, any file missing
  # a comment probably isn't a vorbis file, or it's possible the file is
  # corrupt.
  def exists?
    begin
      fields
      true
    rescue InvalidDataError
      false
    end
  end
  
  # Clear all fields from the vorbis comment
  def remove!
    update{|fields| fields.clear}
  end
  
  # Get all the fields in the vorbis comment
  def fields
    return @fields if @fields
    @fields = CICPHash.new
    read_fields
    @fields
  end
  
  # Return a string suitable for pretty printing, used by the command line
  def pretty_print
    begin
      fields.sort.collect{|key, value| "#{key}: #{value.join(', ')}"}.join("\n")
    rescue OpenError
      "FILE NOT FOUND!"
    rescue InvalidDataError, InvalidCommentError
      "CORRUPT TAG!"
    end
  end
  
  # Update the vorbis comment with the changes that occur to fields inside the
  # block.  Make sure to modify the fields given and not reassign them.
  def update(&block)
    yield fields
    write_fields(normalize_fields)
    fields
  end
  
  private
    # Return a array with key/value pairs so that keys with multiple values
    # have multiple entries.  This is used mainly to make the C extension
    # easier to code.
    def normalize_fields
      comments = []
      fields.each do |key, values|
        key = key.to_s
        raise InvalidCommentError if key =~ BAD_KEY_RE
        values = [values] unless values.is_a?(Array)
        values = values.collect{|value| value.to_s}
        values.each do |value| 
          value.unpack('U*') rescue (raise InvalidCommentError)
          comments << [key, value]
        end
      end
      comments.sort
    end
    
    # Add a value for the given key to the list of values for that key, creating
    # the list if this is the first value for that key.  This is used to make
    # the C extension easier to code.
    def add_to_fields(key, value)
      (fields[key] ||= []) << value
    end
end

require 'vorbis_comment_ext' # define read_fields and write_fields

# If called directly from the command line, treat all arguments as filenames
# and pretty print the vorbis comment fields for each filename.
if __FILE__ == $0
  ARGV.each do |filename| 
    puts filename, '-'*filename.length, VorbisComment.new(filename).pretty_print, ''
  end
end
