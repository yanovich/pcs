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
	long			value;
};

static void
const_run(struct block *b, struct server_state *s)
{
	struct const_state *d = b->data;
	*b->outputs = d->value;
	debug3("%s: %li at %p\n", b->name, d->value, b->outputs);
}

static struct block_ops ops = {
	.run		= const_run,
};

static struct block_ops *
init(struct block *b)
{
	return &ops;
}

static void *
alloc(void)
{
	struct const_state *d = xzalloc(sizeof(*d));
	return d;
}

static int
set_value(void *data, long value)
{
	struct const_state *d = data;
	d->value = value;
	debug("value = %li\n", d->value);
	return 0;
}

static struct pcs_map setpoints[] = {
	{
		.key			= NULL,
		.value			= set_value,
	}
	,{
	}
};

static const char *outputs[] = {
	NULL
};

static struct block_builder const_builder = {
	.ops		= init,
	.alloc		= alloc,
	.setpoints	= setpoints,
	.outputs	= outputs,
};

struct block_builder *
load_const_builder(void)
{
	return &const_builder;
}
