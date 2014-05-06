/* t/t2007.c -- test weighted sum block
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

#include <unistd.h>

#include "block.h"
#include "list.h"
#include "weighted-sum.h"
#include "map.h"
#include "serverconf.h"
#include "state.h"

int main(int argc, char **argv)
{
	struct server_state s;
	struct block_builder *bb;
	struct block *b;
	void (*set_input)(void *, const char const *, long *);
	long input[8], res;

	bb = load_weighted_sum_builder();
	b = xzalloc(sizeof(*b));
	b->data = bb->alloc();
	b->ops = bb->ops();
	if (!b->ops || !b->ops->run)
		fatal("t2007: bad 'weighted sum' ops\n");

	if (NULL == bb->inputs)
		fatal("t2007: bad 'weighted sum' input table\n");
	if (NULL != bb->inputs[0].key)
		fatal("t2007: bad 'weighted sum' input key\n");
	if (NULL == bb->inputs[0].value)
		fatal("t2007: bad 'weighted sum' input value\n");
	set_input = bb->inputs[0].value;
	set_input(b->data, "value", &input[0]);
	set_input(b->data, "value", &input[2]);
	set_input(b->data, "value", &input[4]);
	set_input(b->data, "value", &input[6]);

	if (NULL == bb->outputs)
		fatal("t2007: bad 'weighted sum' output table\n");
	if (NULL != bb->outputs[0])
		fatal("t2007: bad 'weighted sum' output count\n");
	b->outputs = &res;

	if (NULL != bb->setpoints)
		fatal("t2007: bad 'weighted sum' setpoints table\n");

	input[0] = 0;
	input[1] = 1;
	input[2] = 2;
	input[3] = 0;
	input[4] = 3;
	input[5] = 0;
	input[6] = 4;
	input[7] = 0;
	res = 1;
	b->ops->run(b, &s);

	if (res != 0)
		fatal("t2007: bad 'weighted sum' value 1 (%li)\n",
				res);

	input[0] = 0;
	input[1] = 0;
	input[2] = 2;
	input[3] = 2;
	input[4] = 4;
	input[5] = 2;
	input[6] = 3;
	input[7] = 1;
	res = 1;
	b->ops->run(b, &s);

	if (res != 3)
		fatal("t2007: bad 'weighted sum' value 2 (%li)\n",
				res);

	return 0;
}
