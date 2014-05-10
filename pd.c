/* pd.c -- calculate Proportional error and Differential
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
#include "pd.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"pd"

struct pd_state {
	long			*feed;
	long			*reference;
	int			first_run;
	long			prev;
};

static void
pd_run(struct block *b, struct server_state *s)
{
	struct pd_state *d = b->data;
	if (d->first_run) {
		d->first_run = 0;
		d->prev = *d->feed;
	}
	b->outputs[0] = *d->feed - *d->reference;
	b->outputs[1] = *d->feed - d->prev;
	d->prev = *d->feed;
}

static void
set_feed(void *data, const char const *key, long *input)
{
	struct pd_state *d = data;
	d->feed = input;
}

static void
set_reference(void *data, const char const *key, long *input)
{
	struct pd_state *d = data;
	d->reference = input;
}

static const char *outputs[] = {
	"error",
	"diff",
	NULL
};

static struct pcs_map inputs[] = {
	{
		.key			= "feed",
		.value			= set_feed,
	}
	,{
		.key			= "reference",
		.value			= set_reference,
	}
	,{
	}
};

static void *
alloc(void)
{
	struct pd_state *d = xzalloc(sizeof(*d));
	d->first_run = 1;
	return d;
}

static struct block_ops ops = {
	.run		= pd_run,
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
load_pd_builder(void)
{
	return &builder;
}
