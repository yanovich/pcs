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
	long			stage_mark;
	long			next_stage;
	long			unstage_interval;
	long			unstage_mark;
	long			next_unstage;
};

static void
cascade_run(struct block *b, struct server_state *s)
{
	struct cascade_state *d = b->data;
	int i, j;

	if (d->unstage_mark)
		d->unstage_mark--;
	if (d->stage_mark)
		d->stage_mark--;

	if (d->input_stop && *d->input_stop) {
		for (i = 0; i < d->output_count; i++)
			if (b->outputs[i]) {
				b->outputs[i] = 0;
				d->unstage_mark = d->unstage_interval;
			}
		return;
	}
	if (*d->input_stage) {
		if (d->stage_mark)
			return;
		for (i = 0; i < d->output_count; i++) {
			j = (i + d->next_stage) % d->output_count;
			if (!b->outputs[j]) {
				b->outputs[j] = 1;
				d->stage_mark = d->stage_interval;
				d->next_stage = j + 1;
				break;
			}
		}
	} else if (*d->input_unstage) {
		if (d->unstage_mark)
			return;
		for (i = 0; i < d->output_count; i++) {
			j = (i + d->next_unstage) % d->output_count;
			if (b->outputs[j]) {
				b->outputs[j] = 0;
				d->unstage_mark = d->unstage_interval;
				d->next_unstage = j + 1;
				break;
			}
		}
	}
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
set_output_count(void *data, const char const *key, long value)
{
	struct cascade_state *d = data;

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
	debug("output count = %li\n", d->output_count);
	return 0;
}

static int
set_stage_interval(void *data, const char const *key, long value)
{
	struct cascade_state *d = data;
	d->stage_interval = value;
	debug("stage_interval = %li\n", d->stage_interval);
	return 0;
}

static int
set_unstage_interval(void *data, const char const *key, long value)
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
init(struct block *b)
{
	struct cascade_state *d = b->data;
	int first;
	char buff[16];
	int i;

	if (0 == d->output_count) {
		error("%s: bad output count\n", PCS_BLOCK);
		return NULL;
	}
	if (NULL == d->input_stage) {
		error("%s: bad stage input\n", PCS_BLOCK);
		return NULL;
	}
	if (NULL == d->input_unstage) {
		error("%s: bad unstage input\n", PCS_BLOCK);
		return NULL;
	}
	first = rand() % d->output_count;
	d->next_stage = first;
	d->next_unstage = first;
	b->outputs_table = xzalloc(sizeof(*b->outputs_table) *
			(d->output_count + 1));
	for (i = 0; i < d->output_count; i++) {
		snprintf(buff, 16, "%i", i + 1);
		b->outputs_table[i] = strdup(buff);
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
