/* i-87040.c -- load I-87040 status
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
#include "i-87040.h"
#include "icpdas.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"i-87040"

static const char const *default_device = "/dev/ttyS1";

struct i_87040_state {
	unsigned		slot;
	const char		*device;
};

static void
i_87040_run(struct block *b, struct server_state *s)
{
	struct i_87040_state *d = b->data;
	unsigned long di32;
	int err, i;

	err = icpdas_get_serial_digital_input(d->device, d->slot, &di32);
	if (0 > err)
		error("bad i-87040 input in slot %u\n", d->slot);

	debug3("%s: i-87040 di 0x%08lx\n",
			b->name, di32);

	for (i = 0; i < 32; i++) {
		b->outputs[i] = di32 & 1;
		di32 >>= 1;
	}
}

static int
set_slot(void *data, long value)
{
	struct i_87040_state *d = data;
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
	"di0",
	"di1",
	"di2",
	"di3",
	"di4",
	"di5",
	"di6",
	"di7",
	"di8",
	"di9",
	"di10",
	"di11",
	"di12",
	"di13",
	"di14",
	"di15",
	"di16",
	"di17",
	"di18",
	"di19",
	"di20",
	"di21",
	"di22",
	"di23",
	"di24",
	"di25",
	"di26",
	"di27",
	"di28",
	"di29",
	"di30",
	"di31",
	NULL
};

static void *
i_87040_alloc(void)
{
	struct i_87040_state *d = xzalloc(sizeof(*d));
	d->device = default_device;
	return d;
}

static struct block_ops i_87040_ops = {
	.run		= i_87040_run,
};

static struct block_ops *
i_87040_init(void *data)
{
	return &i_87040_ops;
}

static struct block_builder i_87040_builder = {
	.alloc		= i_87040_alloc,
	.ops		= i_87040_init,
	.setpoints	= setpoints,
	.outputs	= outputs,
};

struct block_builder *
load_i_87040_builder(void)
{
	return &i_87040_builder;
}
