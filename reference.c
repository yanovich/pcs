/* reference.c -- process control service
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
#include "reference.h"

struct reference_config {
	int			low_ref;
	int			high_ref;
	int			low;
	int			high;
	int			reference;
	int			feedback;
	struct valve		*valve;
};

static void
reference_run(struct site_status *s, void *conf)
{
	struct reference_config *c = conf;
	int pct = 10000 * (s->AI[c->reference] - c->low_ref) / (c->high_ref - c->low_ref);
	int val = c->low + (pct * (c->high - c->low)) / 10000;

	debug("running reference\n");
	debug("  val %i\n", val);
	val -= s->AI[c->feedback];
	if (!c->valve || !c->valve->ops || !c->valve->ops->adjust)
		fatal("bad valve\n");
	
	c->valve->ops->adjust(val, c->valve->data, s);
	return;
}

static int
reference_log(struct site_status *s, void *conf, char *buff,
		const int sz, int o)
{
	struct reference_config *c = conf;
	int b = 0;
	if (o == sz)
		return 0;
	if (o) {
		buff[o] = ',';
		b++;
	}
	if (c->valve && c->valve->ops && c->valve->ops->log)
		b += c->valve->ops->log(c->valve->data, &buff[o + b],
			       	sz, o + b);

	return b;
}

struct process_ops reference_ops = {
	.run = reference_run,
	.log = reference_log,
};

static void *
reference_init(void *conf)
{
	return &reference_ops;
}

static void
set_low_ref(void *conf, int value)
{
	struct reference_config *c = conf;
	c->low_ref = value;
	debug("  reference: low_ref = %i\n", value);
}

static void
set_high_ref(void *conf, int value)
{
	struct reference_config *c = conf;
	c->high_ref = value;
	debug("  reference: high_ref = %i\n", value);
}

static void
set_low(void *conf, int value)
{
	struct reference_config *c = conf;
	c->low = value;
	debug("  reference: low = %i\n", value);
}

static void
set_high(void *conf, int value)
{
	struct reference_config *c = conf;
	c->high = value;
	debug("  reference: high = %i\n", value);
}

struct setpoint_map reference_setpoints[] = {
	{
		.name 		= "low_ref",
		.set		= set_low_ref,
	},
	{
		.name 		= "high_ref",
		.set		= set_high_ref,
	},
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
set_reference(void *conf, int type, int value)
{
	struct reference_config *c = conf;
	if (type != AI_MODULE)
		fatal("reference: wrong type of reference sensor\n");
	c->reference = value;
	debug("  reference: reference = %i\n", value);
}

static void
set_feedback(void *conf, int type, int value)
{
	struct reference_config *c = conf;
	if (type != AI_MODULE)
		fatal("reference: wrong type of feedback sensor\n");
	c->feedback = value;
	debug("  reference: feedback = %i\n", value);
}

struct io_map reference_io[] = {
	{
		.name 		= "reference",
		.set		= set_reference,
	},
	{
		.name 		= "feedback",
		.set		= set_feedback,
	},
	{
	}
};

static void *
reference_alloc(void)
{
	struct reference_config *c = xzalloc(sizeof(*c));
	return c;
}

static void
set_valve(void *conf, struct valve *v)
{
	struct reference_config *c = conf;
	c->valve = v;
}

struct process_builder reference_builder = {
	.alloc			= reference_alloc,
	.setpoint		= reference_setpoints,
	.io			= reference_io,
	.set_valve		= set_valve,
	.ops			= reference_init,
};

struct process_builder *
load_reference_builder(void)
{
	return &reference_builder;
}
