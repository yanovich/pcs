/* heat-counter.c -- load status from a file
 * Copyright (C) 2016 Sergei Ianovich <ynvich@gmail.com>
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
#include "heat-counter.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"heat-counter"

struct heat_counter_state {
	long			*trigger;
	long			*supply;
	long			*flyback;
	const char		*path;
	int			first;
	long			state;
	long			ticks;
};

static void
save_file(struct block *b)
{
	struct heat_counter_state *d = b->data;
	FILE *f;

	if (!d->path)
		return;

        f = fopen(d->path, "wb");
	if (!f) {
		error("%s: failed to open '%s'\n", PCS_BLOCK, d->path);
		return;
	}
	fwrite(b->outputs, sizeof(b->outputs[0]), 2, f);
	fclose(f);
}

static void
load_file(struct block *b)
{
	struct heat_counter_state *d = b->data;
	FILE *f;
	int items;

	if (!d->path)
		return;

        f = fopen(d->path, "rb");
	if (!f) {
		error("%s: failed to open '%s'\n", PCS_BLOCK, d->path);
		return;
	}

	items = fread(b->outputs, sizeof(b->outputs[0]), 2, f);
	if (items != 2) {
		error("%s: failed to read from '%s'\n", PCS_BLOCK, d->path);
		b->outputs[0] = 0;
		b->outputs[1] = 0;
	}

	fclose(f);
}

static void
heat_counter_run(struct block *b, struct server_state *s)
{
	struct heat_counter_state *d = b->data;
	long tick_msec;
	long energy;

	tick_msec = s->tick.tv_sec * 1000;
	tick_msec += s->tick.tv_usec / 1000;
	tick_msec *= b->multiple;

	if (d->first) {
		d->first = 0;
		load_file(b);
		if (b->outputs[1])
			d->ticks = 60 * 60 * 1000 / 2 / tick_msec
				/ b->outputs[1];
		else
			d->ticks = 1;
	}

	if (!d->state && *d->trigger) {
		energy = ((*d->supply - *d->flyback) * 4200) / 3600;
		b->outputs[0] += energy;
		b->outputs[1] = (energy * 60 * 60 * 1000 / 10) / tick_msec
			/ d->ticks;
		d->ticks = 0;
		save_file(b);
	}

	d->state = *d->trigger;
	d->ticks++;
}

static void
set_trigger(void *data, const char const *key, long *trigger)
{
	struct heat_counter_state *d = data;
	d->trigger = trigger;
}

static void
set_supply(void *data, const char const *key, long *supply)
{
	struct heat_counter_state *d = data;
	d->supply = supply;
}

static void
set_flyback(void *data, const char const *key, long *flyback)
{
	struct heat_counter_state *d = data;
	d->flyback = flyback;
}

static int
set_path(void *data, const char const *key, const char const *value)
{
	struct heat_counter_state *d = data;
	if (d->path) {
		error("%s: 'path' already initialized\n", PCS_BLOCK);
		return 1;
	}
	d->path = strdup(value);
	debug("path = %s\n", d->path);
	return 0;
}

static struct pcs_map inputs[] = {
	{
		.key			= "trigger",
		.value			= set_trigger,
	}
	,{
		.key			= "supply",
		.value			= set_supply,
	}
	,{
		.key			= "flyback",
		.value			= set_flyback,
	}
};

static const char *outputs[] = {
	"count",
	"rate",
	NULL
};

static struct pcs_map strings[] = {
	{
		.key			= "path",
		.value			= set_path,
	}
	,{
	}
};

static void *
alloc(void)
{
	struct heat_counter_state *d = xzalloc(sizeof(struct heat_counter_state));
	d->first = 1;
	return d;
}

static struct block_ops ops = {
	.run		= heat_counter_run,
};

static struct block_ops *
init(struct block *b)
{
	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.inputs		= inputs,
	.ops		= init,
	.outputs	= outputs,
	.strings	= strings,
};

struct block_builder *
load_heat_counter_builder(void)
{
	return &builder;
}
