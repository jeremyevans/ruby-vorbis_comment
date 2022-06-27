#!/usr/bin/env ruby

require 'cicphash'

# Contains all of the Vorbis comment fields found in the filename/file given.
class VorbisComment
  attr_reader :filename
  BAD_KEY_RE = /[\x00-\x1f=\x7e-\xff]/n
  
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

require_relative 'vorbis_comment_ext' # define read_fields and write_fields

# :nocov:
# If called directly from the command line, treat all arguments as filenames
# and pretty print the vorbis comment fields for each filename.
if __FILE__ == $0
  ARGV.each do |filename| 
    puts filename, '-'*filename.length, VorbisComment.new(filename).pretty_print, ''
  end
end
# :nocov:
