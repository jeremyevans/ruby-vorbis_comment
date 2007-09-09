/*
 * Copyright (C) 2000-2001 Michael Smith (msmith at xiph org)
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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <assert.h>

#include "vcedit.h"

#define CHUNKSIZE 4096

struct vcedit_state_St {
	int refcount;

	ogg_sync_state oy;
	ogg_stream_state os;

	vorbis_comment vc;
	vorbis_info vi;

	FILE *in;
	mode_t file_mode;

	bool opened;
	long serial;

	ogg_packet packet_main;
	ogg_packet packet_code_books;

	char *vendor;
	int prevW;

	bool extra_page;
	bool eos;

	char filename[0];
};

static void
ogg_packet_init (ogg_packet *p, unsigned char *buf, long len)
{
	p->packet = buf;
	p->bytes = len;

	p->b_o_s = p->e_o_s = 0;
	p->granulepos = p->packetno = 0;
}

static bool
ogg_packet_dup_data (ogg_packet *p)
{
	unsigned char *tmp;

	tmp = malloc (p->bytes);
	if (!tmp)
		return false;

	memcpy (tmp, p->packet, p->bytes);
	p->packet = tmp;

	return true;
}

static void
vcedit_state_free (vcedit_state *s)
{
	free (s->vendor);

	if (s->in) {
		fclose (s->in);
		s->in = NULL;
	}

	free (s);
}

static bool
vcedit_state_init (vcedit_state *s, const char *filename)
{
	s->refcount = 1;

	ogg_packet_init (&s->packet_main, NULL, 0);
	ogg_packet_init (&s->packet_code_books, NULL, 0);

	strcpy (s->filename, filename);

	return true;
}

vcedit_state *
vcedit_state_new (const char *filename)
{
	vcedit_state *s;
	size_t len;

	len = strlen (filename);
	if (len > PATH_MAX)
		return NULL;

	s = malloc (sizeof (vcedit_state) + len + 1);
	if (!s)
		return NULL;

	memset (s, 0, sizeof (vcedit_state));

	if (!vcedit_state_init (s, filename)) {
		vcedit_state_free (s);
		return NULL;
	}

	return s;
}

vorbis_comment *
vcedit_comments (vcedit_state *s)
{
	return s->opened ? &s->vc : NULL;
}

static void
vcedit_clear_internals (vcedit_state *s)
{
	ogg_stream_clear (&s->os);
	ogg_sync_clear (&s->oy);

	vorbis_info_clear (&s->vi);
	vorbis_comment_clear (&s->vc);

	free (s->vendor);
	s->vendor = NULL;

	ogg_packet_clear (&s->packet_main);
	ogg_packet_clear (&s->packet_code_books);

	s->serial = 0;
	s->opened = false;
}

void
vcedit_state_ref (vcedit_state *s)
{
	s->refcount++;
}

void
vcedit_state_unref (vcedit_state *s)
{
	if (--s->refcount)
		return;

	if (s->opened)
		vcedit_clear_internals (s);

	vcedit_state_free (s);
}

/* Next two functions pulled straight from libvorbis, apart from one change
 * - we don't want to overwrite the vendor string.
 */
static void
_v_writestring (oggpack_buffer *o, const char *s, int len)
{
	while (len--)
		oggpack_write (o, *s++, 8);
}

static bool
write_comments (vcedit_state *s, ogg_packet *packet)
{
	oggpack_buffer opb;
	size_t len;
	int i;

	ogg_packet_init (packet, NULL, 0);

	oggpack_writeinit (&opb);

	/* preamble */
	oggpack_write (&opb, 0x03, 8);
	_v_writestring (&opb, "vorbis", 6);

	/* vendor */
	len = strlen (s->vendor);
	oggpack_write (&opb, len, 32);
	_v_writestring (&opb, s->vendor, len);

	/* comments */
	oggpack_write (&opb, s->vc.comments, 32);

	for (i = 0; i < s->vc.comments; i++)
		if (!s->vc.user_comments[i])
			oggpack_write (&opb, 0, 32);
		else {
			oggpack_write (&opb, s->vc.comment_lengths[i], 32);
			_v_writestring (&opb, s->vc.user_comments[i],
			                s->vc.comment_lengths[i]);
		}

	oggpack_write (&opb, 1, 1);

	packet->bytes = oggpack_bytes (&opb);

	packet->packet = _ogg_malloc (packet->bytes);
	if (!packet->packet)
		return false;

	memcpy (packet->packet, opb.buffer, packet->bytes);

	oggpack_writeclear (&opb);

	return true;
}

static int
_blocksize (vcedit_state *s, ogg_packet *p)
{
	int this, ret = 0;

	this = vorbis_packet_blocksize (&s->vi, p);

	if (s->prevW)
		ret = (this + s->prevW) / 4;

	s->prevW = this;

	return ret;
}

static int
_fetch_next_packet (vcedit_state *s, ogg_packet *p, ogg_page *page)
{
	char *buffer;
	int result, bytes;

	result = ogg_stream_packetout (&s->os, p);
	if (result == 1)
		return 1;

	if (s->eos)
		return 0;

	while (ogg_sync_pageout (&s->oy, page) != 1) {
		buffer = ogg_sync_buffer (&s->oy, CHUNKSIZE);
		bytes = fread (buffer, 1, CHUNKSIZE, s->in);
		ogg_sync_wrote (&s->oy, bytes);

		if (!bytes && (feof (s->in) || ferror (s->in)))
			return 0;
	}

	if (ogg_page_eos (page))
		s->eos = true;
	else if (ogg_page_serialno (page) != s->serial) {
		s->eos = true;
		s->extra_page = true;

		return 0;
	}

	ogg_stream_pagein (&s->os, page);

	return _fetch_next_packet (s, p, page);
}

vcedit_error
vcedit_open (vcedit_state *s)
{
	vcedit_error ret;
	ogg_packet packet_comments, *header = &packet_comments;
	ogg_page page;
	struct stat st;
	char *buffer;
	size_t bytes, total = 0;
	int i = 0;

	s->in = fopen (s->filename, "rb");
	if (!s->in)
		return VCEDIT_ERR_OPEN;

	s->file_mode = stat (s->filename, &st) ? 0664 : st.st_mode;

	ogg_sync_init (&s->oy);

	do {
		/* Bail if we don't find data in the first 40 kB */
		if (feof (s->in) || ferror (s->in) || total >= (CHUNKSIZE * 10)) {
			ogg_sync_clear (&s->oy);

			return VCEDIT_ERR_INVAL;
		}

		buffer = ogg_sync_buffer (&s->oy, CHUNKSIZE);

		bytes = fread (buffer, 1, CHUNKSIZE, s->in);
		total += bytes;

		ogg_sync_wrote (&s->oy, bytes);
	} while (ogg_sync_pageout (&s->oy, &page) != 1);

	s->serial = ogg_page_serialno (&page);

	ogg_stream_init (&s->os, s->serial);
	vorbis_info_init (&s->vi);
	vorbis_comment_init (&s->vc);

	if (ogg_stream_pagein (&s->os, &page) < 0) {
		ret = VCEDIT_ERR_INVAL;
		goto err;
	}

	if (ogg_stream_packetout (&s->os, &s->packet_main) != 1) {
		ret = VCEDIT_ERR_INVAL;
		goto err;
	}

	if (!ogg_packet_dup_data (&s->packet_main)) {
		s->packet_main.packet = NULL;
		ret = VCEDIT_ERR_INVAL;
		goto err;
	}

	if (vorbis_synthesis_headerin (&s->vi, &s->vc, &s->packet_main) < 0) {
		ret = VCEDIT_ERR_INVAL;
		goto err;
	}

	ogg_packet_init (&packet_comments, NULL, 0);

	while (i < 2) {
		if (feof (s->in) || ferror (s->in)) {
			ret = VCEDIT_ERR_INVAL;
			goto err;
		}

		while (i < 2) {
			int result;

			result = ogg_sync_pageout (&s->oy, &page);
			if (!result)
				break; /* Too little data so far */

			if (result != 1)
				continue;

			ogg_stream_pagein (&s->os, &page);

			while (i < 2) {
				result = ogg_stream_packetout (&s->os, header);
				if (!result)
					break;

				if (result != 1) {
					ret = VCEDIT_ERR_INVAL;
					goto err;
				}

				if (i++ == 1 && !ogg_packet_dup_data (header)) {
					header->packet = NULL;
					ret = VCEDIT_ERR_INVAL;
					goto err;
				}

				vorbis_synthesis_headerin (&s->vi, &s->vc, header);

				header = &s->packet_code_books;
			}
		}

		buffer = ogg_sync_buffer (&s->oy, CHUNKSIZE);

		bytes = fread (buffer, 1, CHUNKSIZE, s->in);
		ogg_sync_wrote (&s->oy, bytes);
	}

	/* Copy the vendor tag */
	s->vendor = strdup (s->vc.vendor);

	/* Headers are done! */
	s->opened = true;

	return VCEDIT_ERR_SUCCESS;

err:
	vcedit_clear_internals (s);

	return ret;
}

static bool
write_data (const void *buf, size_t size, size_t nmemb, FILE *stream)
{
	while (nmemb > 0) {
		size_t w;

		w = fwrite (buf, size, nmemb, stream);
		if (!w && ferror (stream))
			return false;

		nmemb -= w;
		buf += size * w;
	}

	return true;
}

static bool
write_page (FILE *f, ogg_page *p)
{
	return write_data (p->header, 1, p->header_len, f) &&
	       write_data (p->body, 1, p->body_len, f);
}

vcedit_error
vcedit_write (vcedit_state *s)
{
	ogg_stream_state stream;
	ogg_packet packet;
	ogg_page page_out, page_in;
	ogg_int64_t granpos = 0;
	FILE *out;
	char *buffer, tmpfile[PATH_MAX];
	bool success = false, need_flush = false, need_out = false;
	int fd, result, bytes;

	if (!s->opened)
		return VCEDIT_ERR_INVAL;

	strcpy (tmpfile, s->filename);
	strcat (tmpfile, ".XXXXXX");

	fd = mkstemp (tmpfile);
	if (fd == -1)
		return VCEDIT_ERR_TMPFILE;

	out = fdopen (fd, "wb");
	if (!out) {
		unlink (tmpfile);
		close (fd);

		return VCEDIT_ERR_TMPFILE;
	}

	s->prevW = 0;
	s->extra_page = s->eos = false;

	ogg_stream_init (&stream, s->serial);

	/* write "main" packet */
	s->packet_main.b_o_s = 1;
	ogg_stream_packetin (&stream, &s->packet_main);
	s->packet_main.b_o_s = 0;

	/* prepare and write comments */
	if (!write_comments (s, &packet)) {
		ogg_stream_clear (&stream);
		unlink (tmpfile);
		fclose (out);

		return VCEDIT_ERR_INVAL;
	}

	ogg_stream_packetin (&stream, &packet);
	ogg_packet_clear (&packet);

	/* write codebooks */
	ogg_stream_packetin (&stream, &s->packet_code_books);

	while (ogg_stream_flush (&stream, &page_out))
		if (!write_page (out, &page_out))
			goto cleanup;

	while (_fetch_next_packet (s, &packet, &page_in)) {
		bool write = false;
		int size;

		size = _blocksize (s, &packet);
		granpos += size;

		if (need_flush)
			write = ogg_stream_flush (&stream, &page_out);
		else if (need_out)
			write = ogg_stream_pageout (&stream, &page_out);

		if (write && !write_page (out, &page_out))
			goto cleanup;

		need_flush = need_out = false;

		if (packet.granulepos == -1) {
			packet.granulepos = granpos;
			ogg_stream_packetin (&stream, &packet);
		} else {
			/* granulepos is set, validly. Use it, and force a flush to
			 * account for shortened blocks (vcut) when appropriate
			 */
			if (granpos > packet.granulepos) {
				granpos = packet.granulepos;
				ogg_stream_packetin (&stream, &packet);
				need_flush = true;
			} else {
				ogg_stream_packetin (&stream, &packet);
				need_out = true;
			}
		}
	}

	stream.e_o_s = 1;

	while (ogg_stream_flush (&stream, &page_out))
		if (!write_page (out, &page_out))
			goto cleanup;

	if (s->extra_page && !write_page (out, &page_in))
		goto cleanup;

	/* clear it, because not all paths to here do */
	s->eos = false;

	do {
		/* We copy the rest of the stream (other logical streams)
		 * through, a page at a time.
		 */
		while ((result = ogg_sync_pageout (&s->oy, &page_out)))
			if (result == 1 && !write_page (out, &page_out))
				goto cleanup;

		buffer = ogg_sync_buffer (&s->oy, CHUNKSIZE);
		bytes = fread (buffer, 1, CHUNKSIZE, s->in);
		ogg_sync_wrote (&s->oy, bytes);

		if (ferror (s->in))
			goto cleanup;
	} while (bytes || !feof (s->in));

	s->eos = success = true;

cleanup:
	fclose (s->in);

	if (!success) {
		unlink (tmpfile);
		fclose (out);
	} else {
		fclose (out);
		unlink (s->filename);
		rename (tmpfile, s->filename);
		chmod (s->filename, s->file_mode);
	}

	ogg_stream_clear (&stream);

	if (!s->eos)
		return VCEDIT_ERR_INVAL;

	vcedit_clear_internals (s);

	return (vcedit_open (s) == VCEDIT_ERR_SUCCESS) ?
	       VCEDIT_ERR_SUCCESS : VCEDIT_ERR_REOPEN;
}
