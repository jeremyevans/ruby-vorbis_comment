require 'mkmf'
find_library('ogg', 'ogg_sync_init')
find_library('vorbis', 'vorbis_comment_init')
create_makefile("vorbis_comment_ext")
