/* valve.c -- operate valves
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

struct valve_data {
	int			min;
	int			max;
	int			total;
	int			low;
	int			correction;
	int			close;
	int			open;
	int			fully_open;
	int			manual;
	unsigned int		manual_inverted;
	int			remote_manual;
	int			abs;
	int			pos;
};

static void
adjust_2way_valve(int amount, void *data, void *ss)
{
	struct valve_data *d = data;
	struct site_status *s = ss;
	int reset = 0;
	char *reason = "";

	if (d->manual && (get_DI(d->manual) ^ d->manual_inverted)) {
		reset = 1;
		reason = "manual override detected\n";
		debug2("%s", reason);
	}
	if (d->remote_manual && get_DI(d->remote_manual)) {
		reset = 1;
		reason = "remote manual override detected\n";
		debug2("%s", reason);
	}
	if (reset) {
		if (d->abs || d->pos)
			logit("%s", reason);
		d->pos = 0;
		d->abs = 0;
		return;
	}
	debug("amount %5i pos %6i\n", amount, d->pos);
	if (-d->min < amount && amount < d->min)
		return;

	if (d->correction > 150) {
		if (d->pos > (d->total * 150) / d->correction) {
			amount *= d->correction;
			amount /= 100;
			amount *= d->pos;
			amount /= d->total;
			debug("corrected amount %5i\n", amount);
		}
	}

	if (amount > d->max)
		amount = d->max;
	if (amount < -d->max)
		amount = -d->max;

	amount += d->pos;
	if (d->abs == 1) {
		if (amount < 0) {
			amount = 0;
		}
		else if (amount > d->total) {
			amount = d->total;
		}
	} else {
		if (amount <= -d->total) {
			d->pos = d->max;
			amount = 0;
			d->abs = 1;
		}
		else if (amount >= d->total) {
			amount = d->total;
			d->pos = d->total - d->max;
			d->abs = 1;
		}
	}
	if (d->fully_open && d->abs) {
		if (amount <= d->low)
			s->DS[d->fully_open] = 0;
		else if (amount == d->total)
			s->DS[d->fully_open] = 2;
		else
			s->DS[d->fully_open] = 1;
	}
	amount -= d->pos;

	d->pos += amount;
	debug("amount %5i pos %6i\n", amount, d->pos);

	if (-d->min < amount && amount < d->min)
		return;

	if (amount < 0) {
		set_DO(d->open, 0, 0);
		set_DO(d->close, 1, 0);
		set_DO(d->close, 0, amount * -1000);
	} else {
		set_DO(d->close, 0, 0);
		set_DO(d->open, 1, 0);
		set_DO(d->open, 0, amount *  1000);
	}
}

static int
log_2way_valve(void *data, char *buff, const int sz, int c)
{
	struct valve_data *d = data;

	if (c == sz)
		return 0;

	return snprintf(buff, sz - c, "%c %4i",
			d->abs ? 'A' : 'R', d->pos/100);
}

static struct valve_ops valve_ops = {
	.adjust = adjust_2way_valve,
	.log = log_2way_valve,
};

static void *
twv_init(void *conf)
{
	return &valve_ops;
}

static void
set_min(void *conf, int value)
{
	struct valve_data *c = conf;
	c->min = value;
	debug("  2way valve: min = %i\n", value);
}

static void
set_max(void *conf, int value)
{
	struct valve_data *c = conf;
	c->max = value;
	debug("  2way valve: max = %i\n", value);
}

static void
set_total(void *conf, int value)
{
	struct valve_data *c = conf;
	c->total = value;
	debug("  2way valve: total = %i\n", value);
}

static void
set_low(void *conf, int value)
{
	struct valve_data *c = conf;
	c->low = value;
	debug("  2way valve: low = %i\n", value);
}

static void
set_correction(void *conf, int value)
{
	struct valve_data *c = conf;
	c->correction = value;
	debug("  2way valve: correction = %i\n", value);
}

static void
set_manual_inverted(void *conf, int value)
{
	struct valve_data *c = conf;
	if (value != 0 && value != 1)
		warn("%s:%i -- unexpected boolean value %i\n",
				__FILE__, __LINE__, value);
	c->manual_inverted = (value != 0);
	debug("  cascade: run_max = %i\n", value);
}

static struct setpoint_map twv_setpoints[] = {
	{
		.name 		= "min",
		.set		= set_min,
	},
	{
		.name 		= "max",
		.set		= set_max,
	},
	{
		.name 		= "total",
		.set		= set_total,
	},
	{
		.name 		= "low",
		.set		= set_low,
	},
	{
		.name 		= "correction",
		.set		= set_correction,
	},
	{
		.name 		= "manual inverted",
		.set		= set_manual_inverted,
	},
	{
	}
};

static void
set_open_io(void *conf, int type, int value)
{
	struct valve_data *c = conf;
	if (type != DO_MODULE)
		fatal("valve: wrong type of open output\n");
	c->open = value;
	debug("  2way valve: open = %i\n", value);
}

static void
set_close_io(void *conf, int type, int value)
{
	struct valve_data *c = conf;
	if (type != DO_MODULE)
		fatal("valve: wrong type of close output\n");
	c->close = value;
	debug("  2way valve: close = %i\n", value);
}

static void
set_fully_open_io(void *conf, int type, int value)
{
	struct valve_data *c = conf;
	if (type != DI_STATUS)
		fatal("valve: wrong type of fully open output\n");
	c->fully_open = value;
	debug("  2way valve: fully open = %i\n", value);
}

static void
set_manual(void *conf, int type, int value)
{
	struct valve_data *c = conf;
	if (type != DI_MODULE)
		fatal("valve: wrong type of manual output\n");
	c->manual = value;
	debug("  2way valve: manual = %i\n", value);
}

static void
set_remote_manual_io(void *conf, int type, int value)
{
	struct valve_data *c = conf;
	if (type != DI_MODULE)
		fatal("valve: wrong type of remote manual sensor\n");
	c->remote_manual = value;
	debug("  valve: remote_manual io = %i\n", value);
}

static struct io_map twv_io[] = {
	{
		.name 		= "open",
		.set		= set_open_io,
	},
	{
		.name 		= "close",
		.set		= set_close_io,
	},
	{
		.name 		= "fully open",
		.set		= set_fully_open_io,
	},
	{
		.name 		= "manual",
		.set		= set_manual,
	},
	{
		.name 		= "remote manual",
		.set		= set_remote_manual_io,
	},
	{
	}
};

static void *
twv_alloc(void)
{
	struct valve_data *c = xzalloc(sizeof(*c));
	c->correction = 100;
	return c;
}

static struct process_builder twv_builder = {
	.alloc			= twv_alloc,
	.setpoint		= twv_setpoints,
	.io			= twv_io,
	.ops			= twv_init,
};

struct process_builder *
load_2way_valve(void)
{
	return &twv_builder;
}
