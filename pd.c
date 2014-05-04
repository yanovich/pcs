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
	long			*input;
	int			first_run;
	long			target;
	long			prev;
};

static void
pd_run(struct block *b, struct server_state *s)
{
	struct pd_state *d = b->data;
	if (d->first_run) {
		d->first_run = 0;
		d->prev = *d->input;
	}
	b->outputs[0] = *d->input - d->target;
	b->outputs[1] = *d->input - d->prev;
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct pd_state *d = data;
	d->input = input;
}

static int
set_target(void *data, long value)
{
	struct pd_state *d = data;
	d->target = value;
	debug("target = %u\n", d->target);
	return 0;
}

static const char *outputs[] = {
	"error",
	"diff",
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
		.value			= set_target,
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
	.setpoints	= setpoints,
};

struct block_builder *
load_pd_builder(void)
{
	return &builder;
}
