/* mark.c -- mark block and its builder
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
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

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
	char buff[24];
	struct tm tm = *localtime(&s->start.tv_sec);
	struct i_8042_state *d = b->data;
	unsigned long di16, do16;
	int err;

	err = icpdas_get_parallel_input(d->slot, &di16);
	if (0 > err)
		error("bad i-8042 input slot %u\n", d->slot);

	err = icpdas_get_parallel_output(d->slot, &do16);
	if (0 > err)
		error("bad i-8042 output slot %u\n", d->slot);

	strftime(&buff[0], sizeof(buff) - 1, "%b %e %H:%M:%S", &tm);
	debug("%s %s: i-8042.slot%u di 0x%04lx do 0x%04lx\n",
			buff, b->name, d->slot, di16, do16);
}

int
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

static void *
i_8042_alloc(void)
{
	return xzalloc(sizeof(struct i_8042_state));
}

static struct block_ops i_8042_ops = {
	.run		= i_8042_run,
};

static struct block_ops *
i_8042_init(void)
{
	return &i_8042_ops;
}

static struct block_builder i_8042_builder = {
	.alloc		= i_8042_alloc,
	.ops		= i_8042_init,
	.setpoints	= setpoints,
};

struct block_builder *
load_i_8042_builder(void)
{
	return &i_8042_builder;
}
