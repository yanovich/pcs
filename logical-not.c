/* logical-not.c -- calculate logical not
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
#include "logical-not.h"

#define PCS_BLOCK	"logical-not"

struct logical_not_state {
	long			*input;
};

static void
logical_not_run(struct block *b, struct server_state *s)
{
	struct logical_not_state *d = b->data;
	if (*d->input != 0 && *d->input != 1) {
		error("%s: bad boolean (%li)\n", PCS_BLOCK, *d->input);
		return;
	}
	*b->outputs = !*d->input;
}

static int
set_input(void *data, const char const *key, long *input)
{
	struct logical_not_state *d = data;
	if (d->input) {
		error("%s: input already defined\n", PCS_BLOCK);
		return 1;
	}
	d->input = input;
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

static void *
alloc(void)
{
	struct logical_not_state *d = xzalloc(sizeof(*d));
	return d;
}

static struct block_ops ops = {
	.run		= logical_not_run,
};

static struct block_ops *
init(void *data)
{
	struct logical_not_state *d = data;
	if (NULL == d->input) {
		error("%s: input not defined\n", PCS_BLOCK);
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
load_logical_not_builder(void)
{
	return &builder;
}
