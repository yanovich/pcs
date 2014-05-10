/* cascade.c -- operate cascade
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
#include <string.h>

#include "block.h"
#include "cascade.h"
#include "list.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK		"cascade"
#define PCS_C_MAX_OUTPUTS	256

struct cascade_state {
	long			*input_stage;
	long			*input_stop;
	long			*input_unstage;
	long			output_count;
	long			stage_interval;
	long			unstage_interval;
};

static void
cascade_run(struct block *b, struct server_state *s)
{
}

static void
set_input_stage(void *data, const char const *key, long *input)
{
	struct cascade_state *d = data;
	d->input_stage = input;
}

static void
set_input_stop(void *data, const char const *key, long *input)
{
	struct cascade_state *d = data;
	d->input_stop = input;
}

static void
set_input_unstage(void *data, const char const *key, long *input)
{
	struct cascade_state *d = data;
	d->input_unstage = input;
}

static struct block_builder builder;

static int
set_output_count(void *data, long value)
{
	struct cascade_state *d = data;
	char buff[16];
	int i;

	if (value <= 0) {
		error("%s: bad output count (%li)\n", PCS_BLOCK, value);
		return 1;
	}
	if (value > PCS_C_MAX_OUTPUTS) {
		error("%s: output count (%li) is over maximum (%i)\n",
				PCS_BLOCK, value, PCS_C_MAX_OUTPUTS);
		return 1;
	}
	d->output_count = value;
	builder.outputs = xzalloc(sizeof(*builder.outputs) * (value + 1));
	for (i = 0; i < value; i++) {
		snprintf(buff, 16, "%i", i);
		builder.outputs[i] = strdup(buff);
	}
	debug("outputs count = %li\n", d->output_count);
	return 0;
}

static int
set_stage_interval(void *data, long value)
{
	struct cascade_state *d = data;
	d->stage_interval = value;
	debug("stage_interval = %li\n", d->stage_interval);
	return 0;
}

static int
set_unstage_interval(void *data, long value)
{
	struct cascade_state *d = data;
	d->unstage_interval = value;
	debug("unstage_interval = %li\n", d->unstage_interval);
	return 0;
}

static struct pcs_map inputs[] = {
	{
		.key			= "stage",
		.value			= set_input_stage,
	}
	,{
		.key			= "stop",
		.value			= set_input_stop,
	}
	,{
		.key			= "unstage",
		.value			= set_input_unstage,
	}
	,{
	}
};

static struct pcs_map setpoints[] = {
	{
		.key			= "output count",
		.value			= set_output_count
	}
	,{
		.key			= "stage interval",
		.value			= set_stage_interval,
	}
	,{
		.key			= "unstage interval",
		.value			= set_unstage_interval,
	}
	,{
	}
};

static void *
alloc(void)
{
	struct cascade_state *d = xzalloc(sizeof(*d));
	return d;
}

static struct block_ops ops = {
	.run		= cascade_run,
};

static struct block_ops *
init(void *data)
{
	struct cascade_state *d = data;
	if (0 == d->output_count) {
		error("%s: bad output count\n", PCS_BLOCK);
		return NULL;
	}
	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.ops		= init,
	.inputs		= inputs,
	.setpoints	= setpoints,
};

struct block_builder *
load_cascade_builder(void)
{
	return &builder;
}
