/* weighted-sum.c -- calculate weighted sum
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
#include "list.h"
#include "map.h"
#include "state.h"
#include "weighted-sum.h"

#define PCS_BLOCK	"weighted-sum"

struct weighted_sum_state {
	struct list_head	component_list;
};

struct weighted_component {
	struct list_head	component_entry;
	long			*data;
};

static void
weighted_sum_run(struct block *b, struct server_state *s)
{
	struct weighted_sum_state *d = b->data;
	struct weighted_component *c;
	long value = 0;
	unsigned long weight = 0;

	list_for_each_entry(c, &d->component_list, component_entry) {
		weight += (unsigned long) c->data[1];
		if (0 == weight)
			continue;
		value += ((c->data[0] - value) * c->data[1]) / weight;
		debug3("%li %lx -> %li %lx\n", c->data[0], c->data[1],
				value, weight);
	}
	*b->outputs = value;
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct weighted_sum_state *d = data;
	struct weighted_component *c = xzalloc(sizeof(*c));
	c->data = input;
	list_add_tail(&c->component_entry, &d->component_list);
}

static const char *outputs[] = {
	NULL
};

static struct pcs_map inputs[] = {
	{
		.key			= NULL,
		.value			= set_input,
	}
};

static void *
alloc(void)
{
	struct weighted_sum_state *d = xzalloc(sizeof(*d));
	INIT_LIST_HEAD(&d->component_list);
	return d;
}

static struct block_ops ops = {
	.run		= weighted_sum_run,
};

static struct block_ops *
init(void)
{
	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.ops		= init,
	.outputs	= outputs,
	.inputs		= inputs,
};

struct block_builder *
load_weighted_sum_builder(void)
{
	return &builder;
}
