/* timer.c -- operate a timer
 * Copyright (C) 2014,2015 Sergei Ianovich <s@asutp.io>
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
#include "timer.h"
#include "list.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"timer"

struct timer_state {
	long			*input;
	long			delay;
	long			mark;
};

static void
timer_run(struct block *b, struct server_state *s)
{
	struct timer_state *d = b->data;

	if (0 == *d->input) {
		d->mark = d->delay;
		*b->outputs = 0;
		return;
	}
	if (d->mark) {
		d->mark--;
		*b->outputs = 0;
		return;
	}
	*b->outputs = 1;
}

static int
set_delay(void *data, const char const *key, long value)
{
	struct timer_state *d = data;
	d->delay = value;
	debug("delay = %li\n", d->delay);
	return 0;
}

static struct pcs_map setpoints[] = {
	{
		.key			= "delay",
		.value			= set_delay,
	}
	,{
	}
};

static int
set_input(void *data, const char const *key, long *input)
{
	struct timer_state *d = data;
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
	struct timer_state *d = xzalloc(sizeof(*d));
	return d;
}

static struct block_ops ops = {
	.run		= timer_run,
};

static struct block_ops *
init(struct block *b)
{
	struct timer_state *d = b->data;
	d->mark = d->delay;

	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.ops		= init,
	.setpoints	= setpoints,
	.outputs	= outputs,
	.inputs		= inputs,
};

struct block_builder *
load_timer_builder(void)
{
	return &builder;
}
