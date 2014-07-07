/* i-8041.c -- write I-8041 status
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
#include "i-8041.h"
#include "icpdas.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"i-8041"

struct i_8041_out_state {
	unsigned		slot;
	long			*DO[32];
};

static void
i_8041_out_run(struct block *b, struct server_state *s)
{
	struct i_8041_out_state *d = b->data;
	unsigned long state = 0;
	unsigned long do32 = 0;
	int err, i;

	err = icpdas_get_parallel_output(d->slot, &state);
	if (0 > err) {
		error("%s: i-8041 read error %i\n", b->name, err);
		state = 0;
	}

	for (i = 31; i >= 0; i--) {
		if (NULL != d->DO[i])
			do32 |= (*d->DO[i]) & 1;
		else
			do32 |= (state >> i) & 1;
		if (i > 0)
			do32 <<= 1;
	}
	err = icpdas_set_parallel_output(d->slot, do32);
	if (0 > err)
		error("%s: i-8041 output error %i\n", b->name, err);

	debug3("%s: i-8041 do 0x%08lx\n",
			b->name, do32);
}

static int
set_out_slot(void *data, const char const *key, long value)
{
	struct i_8041_out_state *d = data;
	d->slot = (unsigned) value;
	debug("slot = %u\n", d->slot);
	return 0;
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct i_8041_out_state *d = data;
	char *bad;
	long i = strtol(key, &bad, 10);
	if (bad[0] != 0 || i < 0 || i >= 32)
		fatal("i-8041 out: bad input '%s'\n", key);
	d->DO[i] = input;
}

static struct pcs_map out_setpoints[] = {
	{
		.key			= "slot",
		.value			= set_out_slot,
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
i_8041_out_alloc(void)
{
	return xzalloc(sizeof(struct i_8041_out_state));
}

static struct block_ops i_8041_out_ops = {
	.run		= i_8041_out_run,
};

static struct block_ops *
i_8041_out_init(struct block *b)
{
	return &i_8041_out_ops;
}

static struct block_builder i_8041_out_builder = {
	.alloc		= i_8041_out_alloc,
	.ops		= i_8041_out_init,
	.setpoints	= out_setpoints,
	.inputs		= out_inputs,
};

struct block_builder *
load_i_8041_builder(void)
{
	return &i_8041_out_builder;
}
