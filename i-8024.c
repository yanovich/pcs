/* i-8024.c -- write I-8024 status
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
#include "i-8024.h"
#include "icpdas.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"i-8024"

struct i_8024_out_state {
	unsigned		slot;
	unsigned		reset;
	unsigned		reset_counter;
	long			*AO[4];
};

static void
i_8024_out_run(struct block *b, struct server_state *s)
{
	struct i_8024_out_state *d = b->data;
	int err, i;

	if (d->reset) {
		if (0 == d->reset_counter) {
			d->reset_counter = d->reset;
			err = icpdas_reset_parallel_analog_output(d->slot);
			if (0 > err)
				error("%s: i-8024 output error %i\n", b->name, err);
		} else {
			d->reset_counter--;
		}
	}

	for (i = 0; i < 4; i++) {
		if (NULL == d->AO[i])
			continue;
		err = icpdas_set_parallel_analog_output(d->slot, i, *d->AO[i]);
		if (0 > err)
			error("%s: i-8024 output error %i\n", b->name, err);
	}
}

static int
set_out_slot(void *data, const char const *key, long value)
{
	struct i_8024_out_state *d = data;
	d->slot = (unsigned) value;
	debug("slot = %u\n", d->slot);
	return 0;
}

static int
set_reset(void *data, const char const *key, long value)
{
	struct i_8024_out_state *d = data;
	d->reset = (unsigned) value;
	debug("reset = %u\n", d->reset);
	return 0;
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct i_8024_out_state *d = data;
	char *bad;
	long i = strtol(key, &bad, 10);
	if (bad[0] != 0 || i < 0 || i >= 4)
		fatal("i-8024 out: bad input '%s'\n", key);
	d->AO[i] = input;
}

static struct pcs_map out_setpoints[] = {
	{
		.key			= "slot",
		.value			= set_out_slot,
	}
	,{
		.key			= "reset",
		.value			= set_reset,
	}
	,{
	}
};

static struct pcs_map out_inputs[] = {
	{
		.key			= NULL,
		.value			= set_input,
	}
};

static void *
i_8024_out_alloc(void)
{
	return xzalloc(sizeof(struct i_8024_out_state));
}

static struct block_ops i_8024_out_ops = {
	.run		= i_8024_out_run,
};

static struct block_ops *
i_8024_out_init(struct block *b)
{
	return &i_8024_out_ops;
}

static struct block_builder i_8024_out_builder = {
	.alloc		= i_8024_out_alloc,
	.ops		= i_8024_out_init,
	.setpoints	= out_setpoints,
	.inputs		= out_inputs,
};

struct block_builder *
load_i_8024_builder(void)
{
	return &i_8024_out_builder;
}
