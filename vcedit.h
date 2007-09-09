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

#ifndef __VCEDIT_H
#define __VCEDIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

typedef enum {
	VCEDIT_ERR_SUCCESS = 0,
	VCEDIT_ERR_OPEN,
	VCEDIT_ERR_INVAL,
	VCEDIT_ERR_TMPFILE,
	VCEDIT_ERR_REOPEN
} vcedit_error;

typedef struct vcedit_state_St vcedit_state;

vcedit_state *vcedit_state_new (const char *filename);
void vcedit_state_ref (vcedit_state *state);
void vcedit_state_unref (vcedit_state *state);
vorbis_comment *vcedit_comments (vcedit_state *state);
vcedit_error vcedit_open (vcedit_state *state);
vcedit_error vcedit_write (vcedit_state *state);

#ifdef __cplusplus
}
#endif

#endif /* __VCEDIT_H */

