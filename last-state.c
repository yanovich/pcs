/* last-state.c -- preserve variables from the previous iteration
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


#include "block.h"
#include "last-state.h"
#include "map.h"
#include "pcs-parser.h"
#include "state.h"

#define PCS_BLOCK	"last-state"
#define PCS_FI_MAX_INPUTS	256

struct last_state_state {
	long			count;
	struct list_head	key_list;
};

struct state_line {
	struct list_head	key_entry;
	const char		*key;
	int			i;
};

static struct block_builder builder;

static void
last_state_run(struct block *b, struct server_state *s)
{
}

static int
set_key(void *data, const char const *key, const char const *value)
{
	struct last_state_state *d = data;
	struct state_line *c = xzalloc(sizeof(*c));
	c->key = strdup(value);
	list_add_tail(&c->key_entry, &d->key_list);
	debug("key = %s\n", c->key);
	c->i = d->count;
	d->count++;
	return 0;
}

static struct pcs_map strings[] = {
	{
		.key			= "key",
		.value			= set_key,
	}
	,{
	}
};

static void *
alloc(void)
{
	struct last_state_state *d = xzalloc(sizeof(*d));
	INIT_LIST_HEAD(&d->key_list);
	return d;
}

static struct block_ops ops = {
	.run		= last_state_run,
};

static struct block_ops *
init(struct block *b)
{
	struct last_state_state *d = b->data;
	struct state_line *c;
	int i = 0;

	if (0 == d->count) {
		error("%s: no inputs count\n", PCS_BLOCK);
		return NULL;
	}
	if (d->count > PCS_FI_MAX_INPUTS) {
		error("%s: input count (%li) is over maximum (%i)\n",
				PCS_BLOCK, d->count, PCS_FI_MAX_INPUTS);
		return NULL;
	}
	b->outputs_table = xzalloc(sizeof(*b->outputs_table) * (d->count + 1));
	list_for_each_entry(c, &d->key_list, key_entry) {
		b->outputs_table[i] = c->key;
		debug3(" %i %s\n", i, b->outputs_table[i]);
		i++;
	}
	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.ops		= init,
	.strings	= strings,
};

struct block_builder *
load_last_state_builder(void)
{
	return &builder;
}
