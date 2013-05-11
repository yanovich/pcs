/* pcs.c -- process control service
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
#include "fuzzy.h"
#include "log.h"
#include "list.h"
#include "process.h"
#include "valve.h"
#include "hot_water.h"

struct hot_water_config {
	int			feed;
	int			feed_io;
	int			min;
	int			max;
	int			total;
	int			open;
	int			close;
	struct list_head	fuzzy;
	int			first_run;
	int			t21_prev;
	struct valve		*valve;
};

static void
hot_water_run(struct site_status *s, void *conf)
{
	struct hot_water_config *c = conf;
	int vars[2];
	int v21;

	debug("running hot water\n");
	if (c->first_run) {
		c->first_run = 0;
		c->t21_prev = s->T[c->feed_io];
	}
	vars[0] = s->T[c->feed_io] - c->feed;
	vars[1] = s->T[c->feed_io] - c->t21_prev;
	c->t21_prev = s->T[c->feed_io];

	v21 = process_fuzzy(&c->fuzzy, &vars[0]);
	if (!c->valve || !c->valve->ops || !c->valve->ops->adjust)
		fatal("bad valve\n");
	
	c->valve->ops->adjust(v21, c->valve->data);
	return;
}

static int
hot_water_log(struct site_status *s, void *conf, char *buff,
		const int sz, int o)
{
	struct hot_water_config *c = conf;
	int b = 0;

	if (o == sz)
		return 0;
	if (o) {
		buff[o] = ',';
		b++;
	}
	b += snprintf(&buff[o + b], sz - o - b, "T21 %3i ", s->T[c->feed_io]);
	if (c->valve && c->valve->ops && c->valve->ops->log)
		b += c->valve->ops->log(c->valve->data, &buff[o + b],
			       	sz, o + b);

	return b;
}

struct process_ops hot_water_ops = {
	.run = hot_water_run,
	.log = hot_water_log,
};

static void *
hot_water_init(void *conf)
{
	struct hot_water_config *c = conf;

	c->first_run = 1;
	return &hot_water_ops;
}

static void
set_feed(void *conf, int value)
{
	struct hot_water_config *c = conf;
	c->feed = value;
	debug("  hot water: feed = %i\n", value);
}

struct setpoint_map hot_water_setpoints[] = {
	{
		.name 		= "feed",
		.set		= set_feed,
	},
	{
	}
};

static void
set_feed_io(void *conf, int type, int value)
{
	struct hot_water_config *c = conf;
	if (type != TR_MODULE)
		fatal("hot water: wrong type of feed sensor\n");
	c->feed_io = value;
	debug("  hot water: feed_io = %i\n", value);
}

struct io_map hot_water_io[] = {
	{
		.name 		= "feed",
		.set		= set_feed_io,
	},
	{
	}
};

static void *
hwc_alloc(void)
{
	struct hot_water_config *c = xzalloc(sizeof(*c));
	INIT_LIST_HEAD(&c->fuzzy);
	return c;
}

struct list_head *
hot_water_fuzzy(void *conf)
{
	struct hot_water_config *c = conf;
	return &c->fuzzy;
}

static void
set_valve(void *conf, struct valve *v)
{
	struct hot_water_config *c = conf;
	c->valve = v;
}

struct process_builder hot_water_builder = {
	.alloc			= hwc_alloc,
	.setpoint		= hot_water_setpoints,
	.io			= hot_water_io,
	.fuzzy			= hot_water_fuzzy,
	.set_valve		= set_valve,
	.ops			= hot_water_init,
};

struct process_builder *
load_hot_water_builder(void)
{
	return &hot_water_builder;
}
