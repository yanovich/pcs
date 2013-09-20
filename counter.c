/* counter.c -- emulate counter block
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
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>

#include "includes.h"
#include "log.h"
#include "list.h"
#include "process.h"
#include "counter.h"

struct counter_config {
	int			feed;
	int			c;
	int			i;
	int			a;
	int			state;
};

static void
counter_run(struct process *p, struct site_status *s)
{
	struct counter_config *c = p->config;
	int state = get_DI(c->feed);
	c->i++;
	if (c->state && !state) {
		c->c++;
		if (c->c == 1)
			c->i = 0;
		else {
			c->a = (c->c - 1) * 10000;
			c->a /= c->i;
		}
	}
	debug("%i %i %7i\n", c->state, state, c->c);
	c->state = state;
}

static int
counter_log(struct site_status *s, void *conf, char *buff,
		const int sz, int o)
{
	struct counter_config *c = conf;
	int b = 0;

	if (o == sz)
		return 0;
	if (o) {
		buff[o] = ',';
		b++;
	}
	b += snprintf(&buff[o + b], sz - o - b, "C %6i %4i", c->c, c->a);
	return b;
}

struct process_ops counter_ops = {
	.run = counter_run,
	.log = counter_log,
};

static void *
counter_init(void *conf)
{
	return &counter_ops;
}

struct setpoint_map counter_setpoints[] = {
	{
	}
};

static void
set_feed(void *conf, int type, int value)
{
	struct counter_config *c = conf;
	if (type != DI_MODULE)
		fatal("counter: wrong type of feed input\n");
	c->feed = value;
	debug("  counter: feed = %i\n", c->feed);
}

struct io_map counter_io[] = {
	{
		.name 		= "feed",
		.set		= set_feed,
	},
	{
	}
};

static void *
c_alloc(void)
{
	struct counter_config *c = xzalloc(sizeof(*c));
	c->state = 0;
	return c;
}

struct process_builder counter_builder = {
	.alloc			= c_alloc,
	.setpoint		= counter_setpoints,
	.io			= counter_io,
	.ops			= counter_init,
};

struct process_builder *
load_counter_builder(void)
{
	return &counter_builder;
}
