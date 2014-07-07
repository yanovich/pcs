/* pt1000.c -- convert Ohms to grads C
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
#include "pt1000.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"pt1000"

static const long pt1000[] = {
	8031, /*  -50 */
	8427, /*  -40 */
	8822, /*  -30 */
	9216, /*  -20 */
	9609, /*  -10 */
	10000, /*  0 */
	10390, /*  10 */
	10779, /*  20 */
	11167, /*  30 */
	11554, /*  40 */
	11940, /*  50 */
	12324, /*  60 */
	12700, /*  70 */
	13089, /*  80 */
	13470, /*  90 */
	13850, /* 100 */
	14220, /* 110 */
	14606, /* 120 */
	14982, /* 130 */
	15358, /* 140 */
	15731  /* 150 */
};

struct pt1000_state {
	long			*input;
};

static int
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
pt1000_run(struct block *b, struct server_state *s)
{
	struct pt1000_state *d = b->data;
	int i;

	if (!d->input)
		return;
	i = bsearch_interval(pt1000,
			sizeof(pt1000) / sizeof(pt1000[0]),
			*d->input);
	if (i >=0 && i < sizeof(pt1000))
		b->outputs[0] = (100 * (*d->input - pt1000[i])) /
			(pt1000[i + 1] - pt1000[i]) + i * 100 - 500;

	debug("%s: pt1000 %li from %li\n",
			b->name, b->outputs[0], *d->input);
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct pt1000_state *d = data;
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
	return xzalloc(sizeof(struct pt1000_state));
}

static struct block_ops ops = {
	.run		= pt1000_run,
};

static struct block_ops *
init(struct block *b)
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
load_pt1000_builder(void)
{
	return &builder;
}
