/* discrete-valve.c -- operate discrete valve
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
#include "discrete-valve.h"
#include "list.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"discrete-valve"
#define PCS_DV_CLOSE	0
#define PCS_DV_OPEN	1
#define PCS_DV_POSITION	2
#define PCS_DV_ABSOLUTE	3

struct discrete_valve_state {
	long			*input;
	long			*reset;
	long			input_multiple;
	long			span;
	long			c_in;
	long			c_out;
};

static void
discrete_valve_run(struct block *b, struct server_state *s)
{
	struct discrete_valve_state *d = b->data;
	long input = *d->input;
	long tick, span;

	if (d->reset && *d->reset) {
		b->outputs[PCS_DV_CLOSE] = 0;
		b->outputs[PCS_DV_OPEN] = 0;
		b->outputs[PCS_DV_POSITION] = 0;
		b->outputs[PCS_DV_ABSOLUTE] = 0;
		d->c_out = 0;
		if (--d->c_in) {
			d->c_in = d->input_multiple;
			debug3("%s: input (%li) p(%li) reset\n",
					__FUNCTION__,
					(long) 0,
					b->outputs[PCS_DV_POSITION]);
		}
		return;
	}

	if (--d->c_in) {
		if (d->c_out == 0)
			return;
		if (--d->c_out)
			return;
		if (b->outputs[PCS_DV_CLOSE]) {
			b->outputs[PCS_DV_CLOSE] = 0;
			return;
		} else if (b->outputs[PCS_DV_OPEN]) {
			b->outputs[PCS_DV_OPEN] = 0;
			return;
		}
	}
	d->c_in = d->input_multiple;

	tick = s->tick.tv_sec * 1000;
	tick += s->tick.tv_usec / 1000;
	tick *= b->multiple;
	input /= tick;
	span = d->span / tick;

	input += b->outputs[PCS_DV_POSITION];
	if (b->outputs[PCS_DV_ABSOLUTE]) {
		if (input < 0) {
			input = 0;
			if (b->outputs[PCS_DV_POSITION] != 0)
				b->outputs[PCS_DV_POSITION] = d->input_multiple;
		} else if (input > span) {
			input = span;
			if (b->outputs[PCS_DV_POSITION] != span)
				b->outputs[PCS_DV_POSITION] = input - d->input_multiple;
		}
	} else {
		if (input <= -span) {
			b->outputs[PCS_DV_ABSOLUTE] = 1;
			input = 0;
			b->outputs[PCS_DV_POSITION] = d->input_multiple;
		} else if (input >= span) {
			b->outputs[PCS_DV_ABSOLUTE] = 1;
			input = span;
			b->outputs[PCS_DV_POSITION] = input - d->input_multiple;
		}
	}
	input -= b->outputs[PCS_DV_POSITION];
	b->outputs[PCS_DV_POSITION] += input;

	if (input == 0) {
		b->outputs[PCS_DV_CLOSE] = 0;
		b->outputs[PCS_DV_OPEN] = 0;
		d->c_out = 0;
	} else if (input < 0) {
		b->outputs[PCS_DV_CLOSE] = 1;
		b->outputs[PCS_DV_OPEN] = 0;
		d->c_out = -input;
	} else {
		b->outputs[PCS_DV_CLOSE] = 0;
		b->outputs[PCS_DV_OPEN] = 1;
		d->c_out = input;
	}
	if (d->c_out >= d->input_multiple)
		d->c_out = 0;
	debug3("%s: input (%li) p(%li) s(%li) o(%li) c(%li) c_o(%li)\n",
			__FUNCTION__,
			input,
			b->outputs[PCS_DV_POSITION],
			span,
			b->outputs[PCS_DV_OPEN],
			b->outputs[PCS_DV_CLOSE],
			d->c_out);
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct discrete_valve_state *d = data;
	d->input = input;
}

static void
set_reset(void *data, const char const *key, long *input)
{
	struct discrete_valve_state *d = data;
	d->reset = input;
}

static int
set_input_multiple(void *data, const char const *key, long value)
{
	struct discrete_valve_state *d = data;
	d->input_multiple = value;
	debug("input multiple = %li\n", d->input_multiple);
	return 0;
}

static int
set_span(void *data, const char const *key, long value)
{
	struct discrete_valve_state *d = data;
	d->span = value;
	debug("span = %li\n", d->span);
	return 0;
}

static const char *outputs[] = {
	"close",
	"open",
	"position",
	"absolute",
	NULL
};

static struct pcs_map inputs[] = {
	{
		.key			= "input",
		.value			= set_input,
	}
	,{
		.key			= "reset",
		.value			= set_reset,
	}
	,{
	}
};

static struct pcs_map setpoints[] = {
	{
		.key			= "input multiple",
		.value			= set_input_multiple,
	}
	,{
		.key			= "span",
		.value			= set_span,
	}
	,{
	}
};

static void *
alloc(void)
{
	struct discrete_valve_state *d = xzalloc(sizeof(*d));
	d->c_in = 1;
	d->input_multiple = 1;
	return d;
}

static struct block_ops ops = {
	.run		= discrete_valve_run,
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
load_discrete_valve_builder(void)
{
	return &builder;
}
