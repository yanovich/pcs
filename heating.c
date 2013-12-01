/* heating.c -- control heating system temperature
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
#include "heating.h"

struct heating_config {
	int			street_sp;
	int			feed_sp;
	int			flowback_sp;
	int			inside;
	int			stop;
	int			min;
	int			max;
	int			total;
	int			open;
	int			close;
	int			street;
	int			feed;
	int			flowback;
	struct list_head	fuzzy;
	int			first_run;
	int			e11_prev;
	int			e12_prev;
	struct valve		*valve;
};

static void
heating_run(struct process *p, struct site_status *s)
{
	struct heating_config *c = p->config;
	int vars[2];
	int t11, d11, e12, v11;
	int street = s->AI[c->street];

	if (street > c->stop)
		street = c->stop;

	t11 = ((c->feed_sp - c->inside) * (c->inside - street))
		/ (c->inside - c->street_sp) + c->inside;
	d11 = (c->feed_sp - c->flowback_sp) * (c->inside - street)
		/ (c->inside - c->street_sp);
	debug(" t11 %i t12 %i\n", t11, t11 - d11);

	e12 = s->AI[c->flowback] + d11 - t11;

	if (e12 > 0)
		t11 -= e12;
	debug(" t11 %i t12 %i\n", t11, t11 - d11);

	vars[0] = s->AI[c->feed] - t11;
	if (c->first_run) {
		c->first_run = 0;
		c->e11_prev = vars[0];
		c->e12_prev = e12 > 0 ? e12 : 0;
	}
	vars[1] = vars[0] - c->e11_prev;
	if (e12 > 0)
		vars[1] -= e12 - c->e12_prev;
	c->e11_prev = vars[0];
	c->e12_prev = e12 > 0 ? e12 : 0;

	v11 = process_fuzzy(&c->fuzzy, &vars[0]);

	if (!c->valve || !c->valve->ops || !c->valve->ops->adjust)
		fatal("bad valve\n");
	
	c->valve->ops->adjust(v11, c->valve->data, s);
	return;
}

static int
heating_log(struct site_status *s, void *conf, char *buff,
		const int sz, int o)
{
	struct heating_config *c = conf;
	int b = 0;

	if (o == sz)
		return 0;
	if (o) {
		buff[o] = ',';
		b++;
	}
	b += snprintf(&buff[o + b], sz - o - b, "T %3i T11 %3i T12 %3i ",
			s->AI[c->street], s->AI[c->feed], s->AI[c->flowback]);
	if (c->valve && c->valve->ops && c->valve->ops->log)
		b += c->valve->ops->log(c->valve->data, &buff[o + b],
			       	sz, o + b);

	return b;
}

struct process_ops heating_ops = {
	.run = heating_run,
	.log = heating_log,
};

static void *
heating_init(void *conf)
{
	struct heating_config *c = conf;

	c->first_run = 1;
	return &heating_ops;
}

static void
set_feed(void *conf, int value)
{
	struct heating_config *c = conf;
	c->feed_sp = value;
	debug("  heating: feed_sp = %i\n", value);
}

static void
set_flowback(void *conf, int value)
{
	struct heating_config *c = conf;
	c->flowback_sp = value;
	debug("  heating: flowback_sp = %i\n", value);
}

static void
set_street(void *conf, int value)
{
	struct heating_config *c = conf;
	c->street_sp = value;
	debug("  heating: street_sp = %i\n", value);
}

static void
set_inside(void *conf, int value)
{
	struct heating_config *c = conf;
	c->inside = value;
	debug("  heating: inside = %i\n", value);
}

static void
set_stop(void *conf, int value)
{
	struct heating_config *c = conf;
	c->stop = value;
	debug("  heating: stop = %i\n", value);
}

struct setpoint_map heating_setpoints[] = {
	{
		.name 		= "feed",
		.set		= set_feed,
	},
	{
		.name 		= "flowback",
		.set		= set_flowback,
	},
	{
		.name 		= "street",
		.set		= set_street,
	},
	{
		.name 		= "inside",
		.set		= set_inside,
	},
	{
		.name 		= "stop",
		.set		= set_stop,
	},
	{
	}
};

static void
set_feed_io(void *conf, int type, int value)
{
	struct heating_config *c = conf;
	if (type != AI_MODULE)
		fatal("heating: wrong type of feed sensor\n");
	c->feed = value;
	debug("  heating: feed io = %i\n", value);
}

static void
set_flowback_io(void *conf, int type, int value)
{
	struct heating_config *c = conf;
	if (type != AI_MODULE)
		fatal("heating: wrong type of flowback sensor\n");
	c->flowback = value;
	debug("  heating: flowback io = %i\n", value);
}

static void
set_street_io(void *conf, int type, int value)
{
	struct heating_config *c = conf;
	if (type != AI_MODULE)
		fatal("heating: wrong type of street sensor\n");
	c->street = value;
	debug("  heating: street io = %i\n", value);
}

struct io_map heating_io[] = {
	{
		.name 		= "feed",
		.set		= set_feed_io,
	},
	{
		.name 		= "flowback",
		.set		= set_flowback_io,
	},
	{
		.name 		= "street",
		.set		= set_street_io,
	},
	{
	}
};

static void *
hwc_alloc(void)
{
	struct heating_config *c = xzalloc(sizeof(*c));
	INIT_LIST_HEAD(&c->fuzzy);
	return c;
}

struct list_head *
heating_fuzzy(void *conf)
{
	struct heating_config *c = conf;
	return &c->fuzzy;
}

static void
set_valve(void *conf, struct valve *v)
{
	struct heating_config *c = conf;
	c->valve = v;
}

struct process_builder heating_builder = {
	.alloc			= hwc_alloc,
	.setpoint		= heating_setpoints,
	.io			= heating_io,
	.fuzzy			= heating_fuzzy,
	.set_valve		= set_valve,
	.ops			= heating_init,
};

struct process_builder *
load_heating_builder(void)
{
	return &heating_builder;
}
