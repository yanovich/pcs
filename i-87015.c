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
#include "i-87015.h"
#include "icpdas.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"i-87015"

const char const *default_device = "/dev/ttyS1";

struct i_87015_state {
	unsigned		slot;
	const char		*device;
};

static void
i_87015_run(struct block *b, struct server_state *s)
{
	char buff[128];
	struct tm tm = *localtime(&s->start.tv_sec);
	struct i_87015_state *d = b->data;
	long ai[7];
	int err;
	size_t pos;
	int i;

	err = icpdas_get_serial_analog_input(d->device, d->slot, 7, ai);
	if (0 > err)
		error("bad i-87015 input slot %u\n", d->slot);

	pos = strftime(&buff[0], sizeof(buff) - 1, "%b %e %H:%M:%S", &tm);

	pos += snprintf(&buff[pos], 128 - pos, " %s: i-87015 ", b->name);
	for (i = 0; i < 7; i++)
		pos += snprintf(&buff[pos], 128 - pos, "%5li ", ai[i]);
	debug("%s\n", buff);
}

static int
set_slot(void *data, long value)
{
	struct i_87015_state *d = data;
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
i_87015_alloc(void)
{
	struct i_87015_state *d = xzalloc(sizeof(struct i_87015_state));
	d->device = default_device;
	return d;
}

static struct block_ops i_87015_ops = {
	.run		= i_87015_run,
};

static struct block_ops *
i_87015_init(void)
{
	return &i_87015_ops;
}

static struct block_builder i_87015_builder = {
	.alloc		= i_87015_alloc,
	.ops		= i_87015_init,
	.setpoints	= setpoints,
};

struct block_builder *
load_i_87015_builder(void)
{
	return &i_87015_builder;
}
