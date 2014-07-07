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
#include "pcs-parser.h"
#include "state.h"

#define PCS_BLOCK	"file-input"
#define PCS_FI_MAX_INPUTS	256

struct file_input_state {
	long			count;
	const char		*path;
	struct list_head	key_list;
	long			*cache;
};

struct line_key {
	struct list_head	key_entry;
	const char		*key;
	int			i;
	int			present;
};

static struct block_builder builder;

static int
parse_value(struct pcs_parser_node *node, yaml_event_t *event)
{
	const char *key = (const char *) &node[1];
	const char *value = (const char *) event->data.scalar.value;
	struct block *b = node->state->data;
	struct file_input_state *d = b->data;
	struct line_key *c;

	int k = -1, err = 0;
	long val;

	if (YAML_SCALAR_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	list_for_each_entry(c, &d->key_list, key_entry) {
		if (strcmp(c->key, key))
			continue;
		k = c->i;
		break;
	}

	if (k >= 0) {
		val = pcs_parser_long(node, event, &err);
		if (err)
			return 0;
		d->cache[k] = val;
		c->present = 1;
		debug(" %s: %s\n", key, value);
	}
	pcs_parser_remove_node(node);
	return 1;
}

static struct pcs_parser_map top_map[] = {
	{
		.handler		= parse_value,
	}
};

static struct pcs_parser_map document_map = {
	.handler		= pcs_parser_map,
	.data			= &top_map,
};

static struct pcs_parser_map stream_map = {
	.handler		= pcs_parser_one_document,
	.data			= &document_map,
};

static int
load_file(struct block *b, const char const *filename)
{
	struct file_input_state *d = b->data;
	struct line_key *c;
	int err;

	list_for_each_entry(c, &d->key_list, key_entry) {
		c->present = 0;
	}
	err = pcs_parse_yaml(filename, &stream_map, b);
	if (err)
		return 1;
	list_for_each_entry(c, &d->key_list, key_entry) {
		if (1 != c->present)
			return 1;
	}
	memcpy(b->outputs, d->cache, sizeof(*d->cache) * d->count);
	return 0;
}

static void
file_input_run(struct block *b, struct server_state *s)
{
	struct file_input_state *d = b->data;
	b->outputs[d->count] = load_file(b, d->path);
}

static int
set_key(void *data, const char const *key, const char const *value)
{
	struct file_input_state *d = data;
	struct line_key *c = xzalloc(sizeof(*c));
	c->key = strdup(value);
	list_add_tail(&c->key_entry, &d->key_list);
	debug("key = %s\n", c->key);
	c->i = d->count;
	d->count++;
	return 0;
}

static int
set_path(void *data, const char const *key, const char const *value)
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

static struct pcs_map strings[] = {
	{
		.key			= "key",
		.value			= set_key,
	}
	,{
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
	INIT_LIST_HEAD(&d->key_list);
	return d;
}

static struct block_ops ops = {
	.run		= file_input_run,
};

static struct block_ops *
init(struct block *b)
{
	struct file_input_state *d = b->data;
	struct line_key *c;
	int i = 0;

	if (!d->path) {
		error("%s: no input file\n", PCS_BLOCK);
		return NULL;
	}
	if (0 == d->count) {
		error("%s: no inputs count\n", PCS_BLOCK);
		return NULL;
	}
	if (d->count > PCS_FI_MAX_INPUTS) {
		error("%s: input count (%li) is over maximum (%i)\n",
				PCS_BLOCK, d->count, PCS_FI_MAX_INPUTS);
		return NULL;
	}
	b->outputs_table = xzalloc(sizeof(*b->outputs_table) * (d->count + 2));
	list_for_each_entry(c, &d->key_list, key_entry) {
		b->outputs_table[i++] = c->key;
		debug3(" %i %s\n", i - 1, b->outputs_table[i - 1]);
	}
	b->outputs_table[i++] = "error";
	d->cache = xzalloc(sizeof(*d->cache) * d->count);
	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.ops		= init,
	.strings	= strings,
};

struct block_builder *
load_file_input_builder(void)
{
	return &builder;
}
