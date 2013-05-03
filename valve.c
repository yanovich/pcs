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
	int			close;
	int			open;
	int			abs;
	int			pos;
};

static void
adjust_2way_valve(int amount, void *data)
{
	struct valve_data *d = data;
	struct action action = {{0}};
	long usec;

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

	action.type = DIGITAL_OUTPUT;

	d->pos += amount;
	debug("amount %5i pos %6i\n", amount, d->pos);

	if (-d->min < amount && amount < d->min)
		return;

	action.data.digital.mask |= 0x00000030;
	if (amount < 0) {
		usec = amount * -1000;
		action.data.digital.value = 0x00000010;
	} else {
		usec = amount * 1000;
		action.data.digital.value = 0x00000020;
	}

	queue_action(&action);

	action.data.digital.delay = usec;
	action.data.digital.value = 0x00000000;
	queue_action(&action);
}

struct valve_ops valve_ops = {
	.adjust = adjust_2way_valve,
};

struct valve *
load_2way_valve(int min, int max, int total, int close, int open)
{
	struct valve *valve = (void *) xmalloc(sizeof(*valve));
	struct valve_data *d = (void *) xmalloc(sizeof(*d));
	
	valve->ops = &valve_ops;
	valve->data = d;
	d->abs = 0;
	d->min = min;
	d->max = max;
	d->close = close;
	d->open = open;
	d->total = total;

	return valve;
}
