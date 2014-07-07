/* linear.c -- convert input using linear function
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
#include "linear.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"linear"

struct linear_state {
	long			*input;
	long			in_high;
	long			in_low;
	long			out_high;
	long			out_low;
};

static void
linear_run(struct block *b, struct server_state *s)
{
	struct linear_state *d = b->data;
	long long res;

	if (*d->input < d->in_low) {
		*b->outputs = d->out_low;
		return;
	} else if (*d->input > d->in_high) {
		*b->outputs = d->out_high;
		return;
	}
	res  = *d->input - d->in_low;
	res *= d->out_high;
	res /= d->in_high - d->in_low;
	res += d->out_low;

	*b->outputs = (long) res;
	debug3("%s: %lli\n", PCS_BLOCK, res);
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct linear_state *d = data;
	d->input = input;
}

static int
set_in_high(void *data, const char const *key, long value)
{
	struct linear_state *d = data;
	d->in_high = value;
	debug("in_high = %li\n", d->in_high);
	return 0;
}

static int
set_in_low(void *data, const char const *key, long value)
{
	struct linear_state *d = data;
	d->in_low = value;
	debug("in_low = %li\n", d->in_low);
	return 0;
}

static int
set_out_high(void *data, const char const *key, long value)
{
	struct linear_state *d = data;
	d->out_high = value;
	debug("out_high = %li\n", d->out_high);
	return 0;
}

static int
set_out_low(void *data, const char const *key, long value)
{
	struct linear_state *d = data;
	d->out_low = value;
	debug("out_low = %li\n", d->out_low);
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
		.key			= "in high",
		.value			= set_in_high,
	}
	,{
		.key			= "in low",
		.value			= set_in_low,
	}
	,{
		.key			= "out high",
		.value			= set_out_high,
	}
	,{
		.key			= "out low",
		.value			= set_out_low,
	}
	,{
	}
};

static void *
alloc(void)
{
	struct linear_state *d = xzalloc(sizeof(*d));
	return d;
}

static struct block_ops ops = {
	.run		= linear_run,
};

static struct block_ops *
init(struct block *b)
{
	struct linear_state *d = b->data;
	if (d->in_high == d->in_low) {
		error("%s: zero input region\n", PCS_BLOCK);
		return NULL;
	}
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
load_linear_builder(void)
{
	return &builder;
}
