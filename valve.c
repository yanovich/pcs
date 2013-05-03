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
	struct address		close;
	struct address		open;
	int			abs;
	int			pos;
};

static void
adjust_2way_valve(int amount, void *data)
{
	struct valve_data *d = data;

	debug("amount %5i pos %6i\n", amount, d->pos);
	if (-d->min < amount && amount < d->min)
		return;

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
			d->pos = d->total;
		}
	} else {
		if (amount < -d->total) {
			d->pos = d->max;
			amount = 0;
			d->abs = 1;
		}
		else if (amount > d->total) {
			amount = d->total;
			d->pos = d->total - d->max;
			d->abs = 1;
		}
	}
	amount -= d->pos;

	d->pos += amount;
	debug("amount %5i pos %6i\n", amount, d->pos);

	if (-d->min < amount && amount < d->min)
		return;

	if (amount < 0) {
		set_DO(d->close.mod, d->close.i, 1, 0);
		set_DO(d->close.mod, d->close.i, 0, amount * -1000);
	} else {
		set_DO(d->close.mod, d->open.i, 1, 0);
		set_DO(d->close.mod, d->open.i, 0, amount *  1000);
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

struct valve_ops valve_ops = {
	.adjust = adjust_2way_valve,
	.log = log_2way_valve,
};

struct valve *
load_2way_valve(int min, int max, int total, int mc, int c, int mo, int o)
{
	struct valve *valve = (void *) xmalloc(sizeof(*valve));
	struct valve_data *d = (void *) xmalloc(sizeof(*d));
	
	valve->ops = &valve_ops;
	valve->data = d;
	d->abs = 0;
	d->min = min;
	d->max = max;
	d->close.mod = mc;
	d->close.i = c;
	d->open.mod = mo;
	d->open.i = o;
	d->total = total;
	d->pos = 0;

	return valve;
}
