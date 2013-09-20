/* evaporation.c -- control evaporation temperature
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
#include "evaporation.h"

int
linear(int a, int b, int c, int d, int x)
{
	debug2("  linear: %i %i %i %i %i\n", a, b, c, d, x);
	if (x < a)
		return c;
	if (x > b)
		return d;
	return c + ((d - c) * (x - a)) / (b - a);
}

int
r404(int p)
{
	const int o[] =  {814, 1038, 1309, 1632, 2015, 2463, 2986, 3590, 4283,
	       	5074, 5970, 6982, 8118, 9387, 10800, 12366, 14096, 16000,
	       	18090, 20377, 22875};

	int i = sizeof(o) / (2 * sizeof(*o));
	int l = (sizeof(o) / sizeof (*o)) - 2;
	int f = 0;
	do {
		debug2("  r404: %i %i %i\n", i, o[i], p);
		if (o[i] <= p)
			if (o[i + 1] > p) {
				return (50 * (p - o[i])) /
					(o[i + 1] - o[i]) + i * 50 -  500;
			}
			else if (i == l)
				return 500;
			else {
				f = i;
				i = i + 1 + (l - i - 1) / 2;
			}
		else
			if (i == f)
				return -500;
			else {
				l = i;
				i = i - 1 - (i - 1 - f) / 2;
			}
	} while (1);
}

struct evaporation_config {
	int			overheat;
	int			p_sensor_low;
	int			p_sensor_high;
	int			p_low;
	int			p_high;
	int			temperature_io;
	int			pressure_io;
	struct list_head	fuzzy;
	int			first_run;
	int			e_prev;
	int			t;
	int			p;
	struct valve		*valve;
};

static void
evaporation_run(struct process *p, struct site_status *s)
{
	struct evaporation_config *c = p->config;
	int vars[2];
	int pr, v;

	debug("running evaporation\n");
	c->t = s->AI[c->temperature_io];
	pr = linear(c->p_sensor_low, c->p_sensor_high, c->p_low, c->p_high,
			s->AI[c->pressure_io]);
	c->p = r404(pr);
	vars[0] = c->t - c->p - c->overheat;
	debug("  evaporation: %i %i (%i) %i\n", c->t, c->p, pr, vars[0]);
	if (c->first_run) {
		c->first_run = 0;
		c->e_prev = vars[0];
	}
	vars[1] = vars[0] - c->e_prev;
	c->e_prev = vars[0];

	v = process_fuzzy(&c->fuzzy, &vars[0]);
	if (!c->valve || !c->valve->ops || !c->valve->ops->adjust)
		fatal("bad valve\n");
	
	c->valve->ops->adjust(v, c->valve->data, s);
	return;
}

static int
evaporation_log(struct site_status *s, void *conf, char *buff,
		const int sz, int o)
{
	struct evaporation_config *c = conf;
	int b = 0;

	if (o == sz)
		return 0;
	if (o) {
		buff[o] = ',';
		b++;
	}
	b += snprintf(&buff[o + b], sz - o - b, "T %4i P %4i ", c->t, c->p);
	if (c->valve && c->valve->ops && c->valve->ops->log)
		b += c->valve->ops->log(c->valve->data, &buff[o + b],
			       	sz, o + b);

	return b;
}

struct process_ops evaporation_ops = {
	.run = evaporation_run,
	.log = evaporation_log,
};

static void *
evaporation_init(void *conf)
{
	struct evaporation_config *c = conf;

	c->first_run = 1;
	return &evaporation_ops;
}

static void
set_overheat(void *conf, int value)
{
	struct evaporation_config *c = conf;
	c->overheat = value;
	debug("  evaporation: overheat = %i\n", value);
}

static void
set_pressure_sensor_low(void *conf, int value)
{
	struct evaporation_config *c = conf;
	c->p_sensor_low = value;
	debug("  evaporation: pressure sensor low = %i\n", value);
}

static void
set_pressure_sensor_high(void *conf, int value)
{
	struct evaporation_config *c = conf;
	c->p_sensor_high = value;
	debug("  evaporation: pressure sensor high = %i\n", value);
}

static void
set_pressure_low(void *conf, int value)
{
	struct evaporation_config *c = conf;
	c->p_low = value;
	debug("  evaporation: pressure low = %i\n", value);
}

static void
set_pressure_high(void *conf, int value)
{
	struct evaporation_config *c = conf;
	c->p_high = value;
	debug("  evaporation: pressure high = %i\n", value);
}

struct setpoint_map evaporation_setpoints[] = {
	{
		.name 		= "overheat",
		.set		= set_overheat,
	},
	{
		.name 		= "pressure sensor low",
		.set		= set_pressure_sensor_low,
	},
	{
		.name 		= "pressure sensor high",
		.set		= set_pressure_sensor_high,
	},
	{
		.name 		= "pressure low",
		.set		= set_pressure_low,
	},
	{
		.name 		= "pressure high",
		.set		= set_pressure_high,
	},
	{
	}
};

static void
set_temperature_io(void *conf, int type, int value)
{
	struct evaporation_config *c = conf;
	if (type != AI_MODULE)
		fatal("evaporation: wrong type of temperature sensor\n");
	c->temperature_io = value;
	debug("  evaporation: temperature_io = %i\n", value);
}

static void
set_pressure_io(void *conf, int type, int value)
{
	struct evaporation_config *c = conf;
	if (type != AI_MODULE)
		fatal("evaporation: wrong type of feed sensor\n");
	c->pressure_io = value;
	debug("  evaporation: pressure_io = %i\n", value);
}

struct io_map evaporation_io[] = {
	{
		.name 		= "temperature",
		.set		= set_temperature_io,
	},
	{
		.name 		= "pressure",
		.set		= set_pressure_io,
	},
	{
	}
};

static void *
evaporation_alloc(void)
{
	struct evaporation_config *c = xzalloc(sizeof(*c));
	INIT_LIST_HEAD(&c->fuzzy);
	return c;
}

struct list_head *
evaporation_fuzzy(void *conf)
{
	struct evaporation_config *c = conf;
	return &c->fuzzy;
}

static void
set_valve(void *conf, struct valve *v)
{
	struct evaporation_config *c = conf;
	c->valve = v;
}

struct process_builder evaporation_builder = {
	.alloc			= evaporation_alloc,
	.setpoint		= evaporation_setpoints,
	.io			= evaporation_io,
	.fuzzy			= evaporation_fuzzy,
	.set_valve		= set_valve,
	.ops			= evaporation_init,
};

struct process_builder *
load_evaporation_builder(void)
{
	return &evaporation_builder;
}
