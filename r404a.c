/* r404a.c -- convert mbars to grads C
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
#include "r404a.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"r404a"

static const long r404a[] = {
	810,   /* -50 */
	1038,  /* -44 */
	1309,  /* -40 */
	1632,  /* -35 */
	2015,  /* -30 */
	2463,  /* -25 */
	2986,  /* -20 */
	3590,  /* -15 */
	4283,  /* -10 */
	5074,  /* -5  */
	5970,  /*  0  */
	6982,  /*  5  */
	8118,  /* 10  */
	9387,  /* 15  */
	10800, /* 20  */
	12366, /* 25  */
	14096, /* 30  */
	16000, /* 35  */
	18090, /* 40  */
	20377, /* 45  */
	22875  /* 50  */
};

struct r404a_state {
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
r404a_run(struct block *b, struct server_state *s)
{
	struct r404a_state *d = b->data;
	int i;

	if (!d->input)
		return;
	i = bsearch_interval(r404a,
			sizeof(r404a) / sizeof(r404a[0]),
			*d->input);
	if (i >=0 && i < sizeof(r404a))
		b->outputs[0] = (50 * (*d->input - r404a[i])) /
			(r404a[i + 1] - r404a[i]) + i * 50 - 500;

	debug("%s: r404a %li from %li\n",
			b->name, b->outputs[0], *d->input);
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct r404a_state *d = data;
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
	return xzalloc(sizeof(struct r404a_state));
}

static struct block_ops ops = {
	.run		= r404a_run,
};

static struct block_ops *
init(void *data)
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
load_r404a_builder(void)
{
	return &builder;
}
