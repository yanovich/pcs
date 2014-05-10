/* trigger.c -- report value state with hysteresis
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
#include "map.h"
#include "state.h"
#include "trigger.h"

#define PCS_BLOCK	"trigger"

struct trigger_state {
	long			*input;
	long			high;
	long			low;
	long			state;
};

static void
trigger_run(struct block *b, struct server_state *s)
{
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct trigger_state *d = data;
	d->input = input;
}

static int
set_high(void *data, long value)
{
	struct trigger_state *d = data;
	d->high = value;
	debug("high = %li\n", d->high);
	return 0;
}

static int
set_low(void *data, long value)
{
	struct trigger_state *d = data;
	d->low = value;
	if (d->low > d->high) {
		error(PCS_BLOCK ": low bigger than high, set high first\n");
		return 1;
	}
	debug("low = %li\n", d->low);
	return 0;
}

static const char *outputs[] = {
	"high",
	"low",
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
		.key			= "high",
		.value			= set_high,
	}
	,{
		.key			= "low",
		.value			= set_low,
	}
	,{
	}
};

static void *
alloc(void)
{
	struct trigger_state *d = xzalloc(sizeof(*d));
	return d;
}

static struct block_ops ops = {
	.run		= trigger_run,
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
load_trigger_builder(void)
{
	return &builder;
}
