/* copy.c -- operate a copy
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
#include "copy.h"
#include "list.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"copy"

struct copy_state {
	long			*source;
	long			*target;
};

static void
copy_run(struct block *b, struct server_state *s)
{
	struct copy_state *d = b->data;

	*d->target = *d->source;
}

static int
set_source(void *data, const char const *key, long *input)
{
	struct copy_state *d = data;
	d->source = input;
	return 0;
}

static int
set_target(void *data, const char const *key, long *input)
{
	struct copy_state *d = data;
	d->target = input;
	return 0;
}

static struct pcs_map inputs[] = {
	{
		.key			= "source",
		.value			= set_source,
	},
	{
		.key			= "target",
		.value			= set_target,
	},
	{
	}
};

static void *
alloc(void)
{
	struct copy_state *d = xzalloc(sizeof(*d));
	return d;
}

static struct block_ops ops = {
	.run		= copy_run,
};

static struct block_ops *
init(struct block *b)
{
	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.ops		= init,
	.inputs		= inputs,
};

struct block_builder *
load_copy_builder(void)
{
	return &builder;
}
