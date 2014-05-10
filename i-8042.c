/* i-8042.c -- load and write I-8042 status
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
#include "i-8042.h"
#include "icpdas.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"i-8042"

struct i_8042_state {
	unsigned		slot;
};

static void
i_8042_run(struct block *b, struct server_state *s)
{
	struct i_8042_state *d = b->data;
	unsigned long di16, do16;
	int err, i;

	err = icpdas_get_parallel_input(d->slot, &di16);
	if (0 > err)
		error("bad i-8042 input slot %u\n", d->slot);

	err = icpdas_get_parallel_output(d->slot, &do16);
	if (0 > err)
		error("bad i-8042 output slot %u\n", d->slot);

	debug3("%s: i-8042 di 0x%04lx do 0x%04lx\n",
			b->name, di16, do16);

	for (i = 0; i < 16; i++) {
		b->outputs[i] = do16 & 1;
		do16 >>= 1;
	}

	for (; i < 16; i++) {
		b->outputs[i] = di16 & 1;
		di16 >>= 1;
	}
}

static int
set_slot(void *data, long value)
{
	struct i_8042_state *d = data;
	d->slot = (unsigned) value;
	debug("slot = %u\n", d->slot);
	return 0;
}

static struct pcs_map setpoints[] = {
	{
		.key			= "slot",
		.value			= set_slot,
	}
	,{
	}
};

static const char *outputs[] = {
	"do0",
	"do1",
	"do2",
	"do3",
	"do4",
	"do5",
	"do6",
	"do7",
	"do8",
	"do0",
	"do10",
	"do11",
	"do12",
	"do13",
	"do14",
	"do15",
	"di0",
	"di1",
	"di2",
	"di3",
	"di4",
	"di5",
	"di6",
	"di7",
	"di8",
	"di0",
	"di10",
	"di11",
	"di12",
	"di13",
	"di14",
	"di15",
	NULL
};

static void *
i_8042_alloc(void)
{
	return xzalloc(sizeof(struct i_8042_state));
}

static struct block_ops i_8042_ops = {
	.run		= i_8042_run,
};

static struct block_ops *
i_8042_init(void *data)
{
	return &i_8042_ops;
}

static struct block_builder i_8042_builder = {
	.alloc		= i_8042_alloc,
	.ops		= i_8042_init,
	.setpoints	= setpoints,
	.outputs	= outputs,
};

struct block_builder *
load_i_8042_builder(void)
{
	return &i_8042_builder;
}

/* Output */
struct i_8042_out_state {
	unsigned		slot;
	long			*status;
	long			*DO[16];
};

static void
i_8042_out_run(struct block *b, struct server_state *s)
{
	struct i_8042_out_state *d = b->data;
	unsigned long do16 = 0;
	int err, i;

	for (i = 15; i >= 0; i--) {
		if (NULL != d->DO[i])
			do16 |= (*d->DO[i]) & 1;
		else
			do16 |= (d->status[i]) & 1;
		if (i > 0)
			do16 <<= 1;
	}
	err = icpdas_set_parallel_output(d->slot, do16);
	if (0 > err)
		error("%s: i-8042 output error %i\n", b->name, err);

	debug3("%s: i-8042 do 0x%04lx\n",
			b->name, do16);
}

static int
set_out_slot(void *data, long value)
{
	struct i_8042_out_state *d = data;
	d->slot = (unsigned) value;
	debug("slot = %u\n", d->slot);
	return 0;
}

static void
set_out_status(void *data, const char const *key, long *input)
{
	struct i_8042_out_state *d = data;
	d->status = input;
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct i_8042_out_state *d = data;
	char *bad;
	long i = strtol(key, &bad, 10);
	if (bad[0] != 0 || i <=0 || i >= 16)
		fatal("i-8042 out: bad input '%s'\n", key);
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
		.key			= "status",
		.value			= set_out_status,
	}
	,{
		.key			= NULL,
		.value			= set_input,
	}
};

static void *
i_8042_out_alloc(void)
{
	return xzalloc(sizeof(struct i_8042_out_state));
}

static struct block_ops i_8042_out_ops = {
	.run		= i_8042_out_run,
};

static struct block_ops *
i_8042_out_init(void *data)
{
	return &i_8042_out_ops;
}

static struct block_builder i_8042_out_builder = {
	.alloc		= i_8042_out_alloc,
	.ops		= i_8042_out_init,
	.setpoints	= out_setpoints,
	.inputs		= out_inputs,
};

struct block_builder *
load_i_8042_out_builder(void)
{
	return &i_8042_out_builder;
}
