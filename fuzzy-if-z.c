/* fuzzy-if-z.c -- calculate fuzzy logic measure with Z function
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
#include "fuzzy-if-z.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"fuzzy-if-z"

struct fuzzy_if_z_state {
	long			*input;
	long			b;
	long			c;
};

static void
fuzzy_if_z_run(struct block *b, struct server_state *s)
{
	struct fuzzy_if_z_state *d = b->data;
	long x = *d->input;
	if (x >= d->c)
		*b->outputs = 0;
	else if (x <= d->b)
		*b->outputs = 0x10000;
	else
		*b->outputs = (0x10000 * (d->c - x)) / (d->c - d->b);
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct fuzzy_if_z_state *d = data;
	d->input = input;
}

static int
set_b(void *data, const char const *key, long value)
{
	struct fuzzy_if_z_state *d = data;
	d->b = value;
	debug("b = %li\n", d->b);
	return 0;
}

static int
set_c(void *data, const char const *key, long value)
{
	struct fuzzy_if_z_state *d = data;
	d->c = value;
	if (d->b > d->c) {
		error("c cannot be less than b, please set b first\n");
		return 1;
	}
	debug("c = %li\n", d->c);
	return 0;
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

static struct pcs_map setpoints[] = {
	{
		.key			= "b",
		.value			= set_b,
	}
	,{
		.key			= "c",
		.value			= set_c,
	}
	,{
	}
};

static void *
alloc(void)
{
	struct fuzzy_if_z_state *d = xzalloc(sizeof(*d));
	return d;
}

static struct block_ops ops = {
	.run		= fuzzy_if_z_run,
};

static struct block_ops *
init(struct block *b)
{
	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.ops		= init,
	.outputs	= outputs,
	.inputs		= inputs,
	.setpoints	= setpoints,
};

struct block_builder *
load_fuzzy_if_z_builder(void)
{
	return &builder;
}
