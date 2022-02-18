/*
 * Copyright (C) 2007 Jeremy Evans (code at jeremyevans net)
 * Copyright (C) 2006 Tilman Sauerbeck (tilman at code-monkey de)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, version 2.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <ruby.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "vcedit.h"

static VALUE eOpen, eInvalidData, eInvalidComment, eTempFile, eReopen;

/*
 * call-seq:
 *  object.read_fields -> fields
 *
 * Reads the comments from the file into the appropriate data structure.
 * Returns the fields (a precreated CICPHash). Do not call this directly.
 */
VALUE read_fields (VALUE self) {
    vcedit_state *state;
    vorbis_comment *vc;
    VALUE fields, filename;
    VALUE k, v;
    ID add_to_fields;
    int i;

    filename = rb_iv_get(self, "@filename");
    state = vcedit_state_new(StringValuePtr(filename));
    if (!state)
        rb_raise (rb_eNoMemError, "Out of Memory");

    switch (vcedit_open (state)) {
        case VCEDIT_ERR_OPEN:
            vcedit_state_unref(state);
            rb_raise (eOpen, "Cannot open file");
        case VCEDIT_ERR_INVAL:
            vcedit_state_unref(state);
            rb_raise (eInvalidData, "Invalid data");
        default:
            break;
    }

    vc = vcedit_comments(state);
    
    fields = rb_iv_get(self, "@fields");
    rb_funcall(fields, rb_intern("clear"), 0);
    add_to_fields = rb_intern("add_to_fields");

    /* check whether all comments are well-formed */
    for (i = 0; i < vc->comments; i++) {
        char *ptr, *content = vc->user_comments[i];

        ptr = strchr (content, '=');
        if (!ptr) {
            rb_funcall(fields, rb_intern("clear"), 0);
            vcedit_state_unref(state);
            rb_raise (eInvalidComment, "invalid comment - %s", content);
        }
        
        k = rb_str_new (content, ptr - content);
        v = rb_str_new2 (ptr + 1);
        rb_funcall(self, add_to_fields, 2, k, v);
    }
    
    vcedit_state_unref(state);
    return fields;
}

/*
 * call-seq:
 *  object.write_fields(comments) -> comments
 *
 * Writes the comments to the file. Do not call this directly.
 */
VALUE write_fields (VALUE self, VALUE comments) {
    vcedit_state *state;
    vorbis_comment *vc;
    VALUE filename, comment, key, value;
    int i, j;
    long len;

    filename = rb_iv_get(self, "@filename");
    state = vcedit_state_new(StringValuePtr(filename));
    if (!state)
        rb_raise (rb_eNoMemError, "Out of Memory");

    switch (vcedit_open (state)) {
        case VCEDIT_ERR_OPEN:
            vcedit_state_unref(state);
            rb_raise (eOpen, "Cannot open file");
        case VCEDIT_ERR_INVAL:
            vcedit_state_unref(state);
            rb_raise (eInvalidData, "Invalid data");
        default:
            break;
    }

    vc = vcedit_comments(state);
    vorbis_comment_clear(vc);
    vorbis_comment_init(vc);

    len = RARRAY_LEN(comments);
    for (i = 0; i < len; i++) {
        comment = rb_ary_entry(comments, i);
        key = rb_ary_entry(comment, 0);
        value = rb_ary_entry(comment, 1);
        vorbis_comment_add_tag(vc, StringValuePtr(key), StringValuePtr(value));
    }
    
    switch (vcedit_write (state)) {
        case VCEDIT_ERR_INVAL:
            vcedit_state_unref(state);
            rb_raise (eInvalidData, "Invalid data");
        case VCEDIT_ERR_TMPFILE:
            vcedit_state_unref(state);
            rb_raise (eTempFile, "Cannot create temporary file");
        case VCEDIT_ERR_REOPEN:
            vcedit_state_unref(state);
            rb_raise (eReopen, "Cannot reopen file");
        default:
            break;
    }
    vcedit_state_unref(state);
    
    return comments;
}

void Init_vorbis_comment_ext(void) {
    VALUE VorbisComment;
    VorbisComment = rb_define_class("VorbisComment", rb_cObject);
    rb_define_private_method(VorbisComment, "read_fields", read_fields, 0);
    rb_define_private_method(VorbisComment, "write_fields", write_fields, 1);
    eOpen = rb_define_class_under (VorbisComment, "OpenError", rb_eStandardError);
    eInvalidData = rb_define_class_under (VorbisComment, "InvalidDataError", rb_eStandardError);
    eInvalidComment = rb_define_class_under (VorbisComment, "InvalidCommentError", rb_eStandardError);
    eTempFile = rb_define_class_under (VorbisComment, "TempFileError", rb_eStandardError);
    eReopen = rb_define_class_under (VorbisComment, "ReopenError", rb_eStandardError);
}
