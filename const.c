/* const.c -- block to produce constant status value
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
#include "const.h"
#include "map.h"
#include "state.h"

struct const_state {
	long			count;
	struct list_head	key_list;
	int			first;
};

struct line_key {
	struct list_head	key_entry;
	const char		*key;
	long			value;
	int			i;
};

#define PCS_BLOCK	"const"
#define PCS_C_MAX_INPUTS	256

static void
const_run(struct block *b, struct server_state *s)
{
	struct const_state *d = b->data;
	struct line_key *c;
	int i = 0;

	if (!d->first)
		return;
	d->first = 0;
	list_for_each_entry(c, &d->key_list, key_entry) {
		b->outputs[i++] = c->value;
	}
}

static struct block_ops ops = {
	.run		= const_run,
};

static struct block_ops *
init(struct block *b)
{
	struct const_state *d = b->data;
	struct line_key *c;
	int i = 0;

	if (0 == d->count) {
		error("%s: no inputs\n", PCS_BLOCK);
		return NULL;
	}
	if (d->count > PCS_C_MAX_INPUTS) {
		error("%s: input count (%li) is over maximum (%i)\n",
				PCS_BLOCK, d->count, PCS_C_MAX_INPUTS);
		return NULL;
	}
	b->outputs_table = xzalloc(sizeof(*b->outputs_table) * (d->count + 1));
	list_for_each_entry(c, &d->key_list, key_entry) {
		b->outputs_table[i++] = c->key;
		debug3(" %i %s\n", i - 1, b->outputs_table[i - 1]);
	}
	return &ops;
}

static void *
alloc(void)
{
	struct const_state *d = xzalloc(sizeof(*d));
	INIT_LIST_HEAD(&d->key_list);
	d->first = 1;
	return d;
}

static int
set_key(void *data, const char const *key, long value)
{
	struct const_state *d = data;
	struct line_key *c = xzalloc(sizeof(*c));
	c->key = strdup(key);
	c->value = value;
	list_add_tail(&c->key_entry, &d->key_list);
	debug("%s = %li\n", c->key, c->value);
	d->count++;
	return 0;
}

static struct pcs_map setpoints[] = {
	{
		.key			= NULL,
		.value			= set_key,
	}
};

static struct block_builder const_builder = {
	.ops		= init,
	.alloc		= alloc,
	.setpoints	= setpoints,
};

struct block_builder *
load_const_builder(void)
{
	return &const_builder;
}
