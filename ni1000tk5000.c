/* ni1000tk5000.c -- convert Ohms to grads C
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
#include "ni1000tk5000.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"ni1000tk5000"

static const long ni1000tk5000[] = {
	7909, /*  -50 */
	8308, /*  -40 */
	8717, /*  -30 */
	9135, /*  -20 */
	9562, /*  -10 */
	10000, /*  0 */
	10448, /*  10 */
	10907, /*  20 */
	11376, /*  30 */
	11857, /*  40 */
	12350, /*  50 */
	12854, /*  60 */
	13371, /*  70 */
	13901, /*  80 */
	14444, /*  90 */
	15000, /* 100 */
	15570, /* 110 */
	16154, /* 120 */
	16752, /* 130 */
	17365, /* 140 */
	17993  /* 150 */
};

struct ni1000tk5000_state {
	long			*input;
};

int
bsearch_interval(const long *o, int size, long v)
{
	int i = size / 2;
	int l = size - 2;
	int f = 0;
	do {
		if (o[i] <= v)
			if (o[i + 1] > v) {
				return i;
			}
			else if (i == l)
				return size;
			else {
				f = i;
				i = i + 1 + (l - i - 1) / 2;
			}
		else
			if (i == f)
				return -1;
			else {
				l = i;
				i = i - 1 - (i - 1 - f) / 2;
			}
	} while (1);
}

static void
ni1000tk5000_run(struct block *b, struct server_state *s)
{
	struct ni1000tk5000_state *d = b->data;
	int i;

	if (!d->input)
		return;
	i = bsearch_interval(ni1000tk5000,
			sizeof(ni1000tk5000) / sizeof(ni1000tk5000[0]),
			*d->input);
	if (i >=0 && i < sizeof(ni1000tk5000))
		b->outputs[0] = (100 * (*d->input - ni1000tk5000[i])) /
			(ni1000tk5000[i + 1] - ni1000tk5000[i]) + i * 100 - 500;

	debug("%s: ni1000tk5000 %li from %li\n",
			b->name, b->outputs[0], *d->input);
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct ni1000tk5000_state *d = data;
	d->input = input;
}

static struct pcs_map inputs[] = {
	{
		.key			= NULL,
		.value			= set_input,
	}
};

static const char *outputs[] = {
	NULL
};

static void *
alloc(void)
{
	return xzalloc(sizeof(struct ni1000tk5000_state));
}

static struct block_ops ops = {
	.run		= ni1000tk5000_run,
};

static struct block_ops *
init(void)
{
	return &ops;
}

static struct block_builder builder = {
	.alloc		= alloc,
	.ops		= init,
	.inputs		= inputs,
	.outputs	= outputs,
};

struct block_builder *
load_ni1000tk5000_builder(void)
{
	return &builder;
}
