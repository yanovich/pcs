/* analog_valve.c -- operate valves with analog reference
 * Copyright (C) 2013 Sergey Yanovich <ynvich@gmail.com>
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

#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>

#include "includes.h"
#include "log.h"
#include "list.h"
#include "process.h"
#include "valve.h"

#define HAS_FEEDBACK		0x1

struct valve_data {
	int			low;
	int			high;
	int			output;
	int			feedback;
	unsigned		status;
	int			pos;
};

static void
adjust_analog_valve(int amount, void *data, void *s)
{
	struct valve_data *d = data;
	struct site_status *ss = s;

	if (d->status & HAS_FEEDBACK)
		d->pos = ss->AI[d->feedback];
	debug(" analog valve: amount %i pos %i\n", amount, d->pos);
	d->pos += amount;
	if (d->pos > d->high)
		d->pos = d->high;
	if (d->pos < d->low)
		d->pos = d->low;

	set_AO(d->output, d->pos);
}

static int
log_analog_valve(void *data, char *buff, const int sz, int c)
{
	struct valve_data *d = data;

	if (c == sz)
		return 0;

	return snprintf(buff, sz - c, "A %4i",
		       	((d->pos - d->low) * 1000) / (d->high - d->low));
}

static struct valve_ops valve_ops = {
	.adjust = adjust_analog_valve,
	.log = log_analog_valve,
};

static void *
av_init(void *conf)
{
	return &valve_ops;
}

static void
set_low(void *conf, int value)
{
	struct valve_data *c = conf;
	c->low = value;
	debug("  analog valve: low = %i\n", value);
}

static void
set_high(void *conf, int value)
{
	struct valve_data *c = conf;
	c->high = value;
	debug("  analog valve: high = %i\n", value);
}

static struct setpoint_map av_setpoints[] = {
	{
		.name 		= "low",
		.set		= set_low,
	},
	{
		.name 		= "high",
		.set		= set_high,
	},
	{
	}
};

static void
set_output(void *conf, int type, int value)
{
	struct valve_data *c = conf;
	if (type != AO_MODULE)
		fatal("analog valve: wrong type of output\n");
	c->output = value;
	debug("  analog valve: close = %i\n", value);
}

static void
set_feedback(void *conf, int type, int value)
{
	struct valve_data *c = conf;
	if (type != AI_MODULE)
		fatal("analog valve: wrong type of feedback input\n");
	c->feedback = value;
	c->status |= HAS_FEEDBACK;
	debug("  analog valve: feedback = %i\n", value);
}

static struct io_map av_io[] = {
	{
		.name 		= "output",
		.set		= set_output,
	},
	{
		.name 		= "feedback",
		.set		= set_feedback,
	},
	{
	}
};

static void *
av_alloc(void)
{
	struct valve_data *c = xzalloc(sizeof(*c));
	return c;
}

static struct process_builder av_builder = {
	.alloc			= av_alloc,
	.setpoint		= av_setpoints,
	.io			= av_io,
	.ops			= av_init,
};

struct process_builder *
load_analog_valve(void)
{
	return &av_builder;
}
