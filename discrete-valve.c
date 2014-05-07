/* discrete-valve.c -- operate discrete valve
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

#include "block.h"
#include "discrete-valve.h"
#include "list.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"discrete-valve"

struct discrete_valve_state {
	long			*input;
	long			span;
};

static void
discrete_valve_run(struct block *b, struct server_state *s)
{
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct discrete_valve_state *d = data;
	d->input = input;
}

static int
set_span(void *data, long value)
{
	struct discrete_valve_state *d = data;
	d->span = value;
	debug("span = %li\n", d->span);
	return 0;
}

static const char *outputs[] = {
	NULL
};

static struct pcs_map inputs[] = {
	{
		.key			= NULL,
		.value			= set_input,
	}
};

static struct pcs_map setpoints[] = {
	{
		.key			= "span",
		.value			= set_span,
	}
	,{
	}
};

static void *
alloc(void)
{
	struct discrete_valve_state *d = xzalloc(sizeof(*d));
	return d;
}

static struct block_ops ops = {
	.run		= discrete_valve_run,
};

static struct block_ops *
init(void)
{
	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.ops		= init,
	.outputs	= outputs,
	.inputs		= inputs,
	.setpoints	= setpoints,
};

struct block_builder *
load_discrete_valve_builder(void)
{
	return &builder;
}
