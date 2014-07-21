/* trigger.c -- report value state with optional hysteresis
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
#define PCS_T_HIGH	0
#define PCS_T_LOW	1

struct trigger_state {
	long			*high;
	long			*input;
	long			*low;
	long			hysteresis;
};

static void
trigger_run(struct block *b, struct server_state *s)
{
	struct trigger_state *d = b->data;

	if (*d->input >= *d->high) {
		b->outputs[PCS_T_HIGH] = 1;
		b->outputs[PCS_T_LOW] = 0;
		return;
	} else if (*d->input <= *d->low) {
		b->outputs[PCS_T_HIGH] = 0;
		b->outputs[PCS_T_LOW] = 1;
		return;
	}

	if (!d->hysteresis) {
		b->outputs[PCS_T_HIGH] = 0;
		b->outputs[PCS_T_LOW] = 0;
		return;
	}
}

static int
set_high(void *data, const char const *key, long *high)
{
	struct trigger_state *d = data;
	d->high = high;
	return 0;
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct trigger_state *d = data;
	d->input = input;
}

static int
set_low(void *data, const char const *key, long *low)
{
	struct trigger_state *d = data;
	d->low = low;
	return 0;
}

static int
set_hysteresis(void *data, const char const *key, long value)
{
	struct trigger_state *d = data;
	d->hysteresis = value;
	if (value != 0 && value != 1) {
		error(PCS_BLOCK ": bad boolean value (%li)\n", value);
		return 1;
	}
	debug("hysteresis = %li\n", d->hysteresis);
	return 0;
}

static const char *outputs[] = {
	"high",
	"low",
	NULL
};

static struct pcs_map inputs[] = {
	{
		.key			= "high",
		.value			= set_high,
	}
	,{
		.key			= "input",
		.value			= set_input,
	}
	,{
		.key			= "low",
		.value			= set_low,
	}
	,{
	}
};

static struct pcs_map setpoints[] = {
	{
		.key			= "hysteresis",
		.value			= set_hysteresis,
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
load_trigger_builder(void)
{
	return &builder;
}
