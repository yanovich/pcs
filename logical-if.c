/* logical-if.c -- choose using if/then/else
 * Copyright (C) 2015 Sergei Ianovich <ynvich@gmail.com>
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
#include "logical-if.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"logical-if"

struct logical_if_state {
	long			*condition;
	long			*then;
	long			*otherwise;
};

static void
logical_if_run(struct block *b, struct server_state *s)
{
	struct logical_if_state *d = b->data;
	if (*d->condition)
		*b->outputs = *d->then;
	else
		*b->outputs = *d->otherwise;
}

static int
set_condition(void *data, const char const *key, long *input)
{
	struct logical_if_state *d = data;
	d->condition = input;
	return 0;
}

static int
set_then(void *data, const char const *key, long *input)
{
	struct logical_if_state *d = data;
	d->then = input;
	return 0;
}

static int
set_otherwise(void *data, const char const *key, long *input)
{
	struct logical_if_state *d = data;
	d->otherwise = input;
	return 0;
}

static const char *outputs[] = {
	NULL
};

static struct pcs_map inputs[] = {
	{
		.key			= "condition",
		.value			= set_condition,
	}
	,{
		.key			= "then",
		.value			= set_then,
	}
	,{
		.key			= "otherwise",
		.value			= set_otherwise,
	}
	,{
	}
};

static void *
alloc(void)
{
	struct logical_if_state *d = xzalloc(sizeof(*d));
	return d;
}

static struct block_ops ops = {
	.run		= logical_if_run,
};

static struct block_ops *
init(struct block *b)
{
	struct logical_if_state *d = b->data;
	if (NULL == d->condition) {
		error("%s: missing condition input\n", PCS_BLOCK);
		return NULL;
	}
	if (NULL == d->then) {
		error("%s: missing then input\n", PCS_BLOCK);
		return NULL;
	}
	if (NULL == d->otherwise) {
		error("%s: missing otherwise input\n", PCS_BLOCK);
		return NULL;
	}
	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.ops		= init,
	.outputs	= outputs,
	.inputs		= inputs,
};

struct block_builder *
load_logical_if_builder(void)
{
	return &builder;
}
