/* cylinder.c -- convert horizontal axis cylinder tank height to volume
 * Copyright (C) 2016 Sergei Ianovich <ynvich@gmail.com>
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
#include "cylinder.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"cylinder"

static const long cylinder[] = {
	0,    /* 0 */
	17,   /* 1 */
	48,   /* 2 */
	87,   /* 3 */
	134,  /* 4 */
	187,  /* 5 */
	245,  /* 6 */
	308,  /* 7 */
	375,  /* 8 */
	446,  /* 9 */
	520,  /* 10 */
	598,  /* 11 */
	680,  /* 12 */
	764,  /* 13 */
	851,  /* 14 */
	941,  /* 15 */
	1033, /* 16 */
	1127, /* 17 */
	1224, /* 18 */
	1323, /* 19 */
	1424, /* 20 */
	1527, /* 21 */
	1631, /* 22 */
	1738, /* 23 */
	1845, /* 24 */
	1955, /* 25 */
	2066, /* 26 */
	2178, /* 27 */
	2292, /* 28 */
	2407, /* 29 */
	2523, /* 30 */
	2640, /* 31 */
	2759, /* 32 */
	2878, /* 33 */
	2998, /* 34 */
	3119, /* 35 */
	3241, /* 36 */
	3364, /* 37 */
	3487, /* 38 */
	3611, /* 39 */
	3735, /* 40 */
	3860, /* 41 */
	3986, /* 42 */
	4112, /* 43 */
	4238, /* 44 */
	4364, /* 45 */
	4491, /* 46 */
	4618, /* 47 */
	4745, /* 48 */
	4873, /* 49 */
	5000, /* 50 */
	5127, /* 51 */
	5255, /* 52 */
	5382, /* 53 */
	5509, /* 54 */
	5636, /* 55 */
	5762, /* 56 */
	5888, /* 57 */
	6014, /* 58 */
	6140, /* 59 */
	6265, /* 60 */
	6389, /* 61 */
	6513, /* 62 */
	6636, /* 63 */
	6759, /* 64 */
	6881, /* 65 */
	7002, /* 66 */
	7122, /* 67 */
	7241, /* 68 */
	7360, /* 69 */
	7477, /* 70 */
	7593, /* 71 */
	7708, /* 72 */
	7822, /* 73 */
	7934, /* 74 */
	8045, /* 75 */
	8155, /* 76 */
	8262, /* 77 */
	8369, /* 78 */
	8473, /* 79 */
	8576, /* 80 */
	8677, /* 81 */
	8776, /* 82 */
	8873, /* 83 */
	8967, /* 84 */
	9059, /* 85 */
	9149, /* 86 */
	9236, /* 87 */
	9320, /* 88 */
	9402, /* 89 */
	9480, /* 90 */
	9554, /* 91 */
	9625, /* 92 */
	9692, /* 93 */
	9755, /* 94 */
	9813, /* 95 */
	9866, /* 96 */
	9913, /* 97 */
	9952, /* 98 */
	9983, /* 99 */
	10000,/* 100 */
	10000 /* guard */
};

struct cylinder_state {
	long			*input;
};

static void
cylinder_run(struct block *b, struct server_state *s)
{
	struct cylinder_state *d = b->data;
	int i, m;

	if (!d->input)
		return;
	if (*d->input < 0) {
		warn("%s: negative input (%li)\n", b->name, *d->input);
		b->outputs[0] = 0;
		return;
	}
	if (*d->input > 10000) {
		warn("%s: input overflow (%li)\n", b->name, *d->input);
		b->outputs[0] = 10000;
		return;
	}
	i = *d->input / 100;
	m = *d->input % 100;
	b->outputs[0] = cylinder[i];
	b->outputs[0] += (m * (cylinder[i + 1] - cylinder[i])) /100;

	debug("%s: cylinder %li from %li\n",
			b->name, b->outputs[0], *d->input);
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct cylinder_state *d = data;
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
	return xzalloc(sizeof(struct cylinder_state));
}

static struct block_ops ops = {
	.run		= cylinder_run,
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
load_cylinder_builder(void)
{
	return &builder;
}
