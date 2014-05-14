/* file-output.c -- load status from a file
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
#include "file-output.h"
#include "icpdas.h"
#include "map.h"
#include "pcs-parser.h"
#include "state.h"

#define PCS_BLOCK	"file-output"
#define PCS_FI_MAX_INPUTS	256

struct file_output_state {
	const char		*path;
	struct list_head	items;
};

struct log_item {
	struct list_head		item_entry;
	const char			*key;
	long				*value;
};

static void
file_output_run(struct block *b, struct server_state *s)
{
	struct file_output_state *d = b->data;
	struct log_item *item;
	FILE *f;

        f = fopen(d->path, "w");
	list_for_each_entry(item, &d->items, item_entry) {
		fprintf(f, "%s: %li\n", item->key, *item->value);
	}
	fclose(f);
}

static int
set_path(void *data, const char const *key, const char const *value)
{
	struct file_output_state *d = data;
	if (d->path) {
		error("%s: 'path' already initialized\n", PCS_BLOCK);
		return 1;
	}
	d->path = strdup(value);
	debug("path = %s\n", d->path);
	return 0;
}

static struct pcs_map strings[] = {
	{
		.key			= "path",
		.value			= set_path,
	}
	,{
	}
};

static void
set_input(void *data, const char const *key, long *input)
{
	struct file_output_state *d = data;
	struct log_item *item = xzalloc(sizeof(*item));

	if (key)
		item->key = strdup(key);
	item->value = input;
	list_add_tail(&item->item_entry, &d->items);
}

static struct pcs_map inputs[] = {
	{
		.key			= NULL,
		.value			= set_input,
	}
};

static void *
alloc(void)
{
	struct file_output_state *d = xzalloc(sizeof(*d));
	INIT_LIST_HEAD(&d->items);
	return d;
}

static struct block_ops ops = {
	.run		= file_output_run,
};

static struct block_ops *
init(void *data)
{
	struct file_output_state *d = data;

	if (!d->path) {
		error("%s: no input file\n", PCS_BLOCK);
		return NULL;
	}
	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.ops		= init,
	.inputs		= inputs,
	.strings	= strings,
};

struct block_builder *
load_file_output_builder(void)
{
	return &builder;
}
