/* file-input.c -- load status from a file
 * Copyright (C) 2014 Sergei Ianovich <ynvich@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "includes.h"

#include <stdio.h>

#include "block.h"
#include "file-input.h"
#include "icpdas.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"file-input"
#define PCS_FI_MAX_INPUTS	256

static const char const *default_path = "/var/lib/pcs";

struct file_input_state {
	long			count;
	const char		*path;
};

static void
file_input_run(struct block *b, struct server_state *s)
{
}

static struct block_builder builder;

static int
set_count(void *data, long value)
{
	struct file_input_state *d = data;
	char buff[16];
	int i;

	if (value <= 0) {
		error("%s: bad input count (%li)\n", PCS_BLOCK, value);
		return 1;
	}
	if (value > PCS_FI_MAX_INPUTS) {
		error("%s: input count (%li) is over maximum (%i)\n",
				PCS_BLOCK, value, PCS_FI_MAX_INPUTS);
		return 1;
	}
	d->count = value;
	builder.outputs = xzalloc(sizeof(*builder.outputs) * (value + 1));
	for (i = 0; i < value; i++) {
		snprintf(buff, 16, "%i", i);
		builder.outputs[i] = strdup(buff);
	}
	debug("count = %li\n", d->count);
	return 0;
}

static int
set_path(void *data, const char *key, const char *value)
{
	struct file_input_state *d = data;
	if (d->path) {
		error("%s: 'path' already initialized\n", PCS_BLOCK);
		return 1;
	}
	d->path = strdup(value);
	debug("path = %s\n", d->path);
	return 0;
}

static struct pcs_map setpoints[] = {
	{
		.key			= "count",
		.value			= set_count,
	}
	,{
	}
};

static struct pcs_map strings[] = {
	{
		.key			= "path",
		.value			= set_path,
	}
	,{
	}
};

static void *
alloc(void)
{
	struct file_input_state *d = xzalloc(sizeof(struct file_input_state));
	return d;
}

static struct block_ops ops = {
	.run		= file_input_run,
};

static struct block_ops *
init(void *data)
{
	struct file_input_state *d = data;
	if (d->path)
		d->path = default_path;
	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.ops		= init,
	.setpoints	= setpoints,
	.strings	= strings,
};

struct block_builder *
load_file_input_builder(void)
{
	return &builder;
}
