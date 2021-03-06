/* t/t2010.c -- test trigger block
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
#include "map.h"
#include "serverconf.h"
#include "state.h"
#include "trigger.h"

int main(int argc, char **argv)
{
	struct server_state s;
	struct block_builder *bb;
	struct block *b;
	void (*set_high)(void *, const char const *, long *);
	void (*set_input)(void *, const char const *, long *);
	void (*set_low)(void *, const char const *, long *);
	int (*setter)(void *, const char const *, long);
	long input, high = 200, low = 100;
	long res[2];
	int i = 0;

	bb = load_trigger_builder();
	b = xzalloc(sizeof(*b));
	b->data = bb->alloc();
	b->ops = bb->ops(b->data);
	if (!b->ops || !b->ops->run)
		fatal("t2010: bad 'trigger' ops\n");

	if (NULL == bb->inputs)
		fatal("t2010: bad 'trigger' input table\n");
	set_input = pcs_lookup(bb->inputs, "input");
	if (NULL == set_input)
		fatal("t2010: bad 'trigger' input\n");
	set_input(b->data, "input", &input);

	set_high = pcs_lookup(bb->inputs, "high");
	if (NULL == set_high)
		fatal("t2010: bad 'trigger.high' input\n");
	set_high(b->data, "high", &high);

	set_low = pcs_lookup(bb->inputs, "low");
	if (NULL == set_low)
		fatal("t2010: bad 'trigger.low' input\n");
	set_low(b->data, "low", &low);
	if (NULL != bb->inputs[3].key)
		fatal("t2010: bad 'trigger' input count\n");

	if (NULL == bb->outputs)
		fatal("t2010: bad 'trigger' output table\n");
	if (!bb->outputs[0] || strcmp(bb->outputs[0], "high"))
		fatal("t2010: bad 'trigger' output 'high'\n");
	if (!bb->outputs[1] || strcmp(bb->outputs[1], "low"))
		fatal("t2010: bad 'trigger' output 'low'\n");
	if (NULL != bb->outputs[2])
		fatal("t2010: bad 'trigger' output count\n");
	b->outputs = res;

	if (NULL == bb->setpoints)
		fatal("t2010: bad 'trigger' setpoints table\n");

	input = 50;
	res[0] = 1;
	res[1] = 0;
	b->ops->run(b, &s);
	i = 1;

	if (res[0] != 0)
		fatal("t2010:%i: bad 'trigger.high' for %li (%li)\n",
				i, input, res[0]);
	if (res[1] != 1)
		fatal("t2010:%i: bad 'trigger.low' for %li (%li)\n",
				i, input, res[1]);

	input = 150;
	res[0] = 1;
	res[1] = 1;
	b->ops->run(b, &s);
	i = 2;

	if (res[0] != 0)
		fatal("t2010:%i: bad 'trigger.high' for %li (%li)\n",
				i, input, res[0]);
	if (res[1] != 0)
		fatal("t2010:%i: bad 'trigger.low' for %li (%li)\n",
				i, input, res[1]);

	input = 250;
	res[0] = 0;
	res[1] = 1;
	b->ops->run(b, &s);
	i = 3;

	if (res[0] != 1)
		fatal("t2010:%i: bad 'trigger.high' for %li (%li)\n",
				i, input, res[0]);
	if (res[1] != 0)
		fatal("t2010:%i: bad 'trigger.low' for %li (%li)\n",
				i, input, res[1]);

	setter = pcs_lookup(bb->setpoints, "hysteresis");
	if (!setter)
		fatal("t2010: bad 'trigger' setpoint 'hysteresis'\n");
	if (!setter(b->data, "hysteresis", 200))
		fatal("t2010: 'trigger' accepts bad boolean");
	setter(b->data, "hysteresis", 1);

	input = 150;
	res[0] = 1;
	res[1] = 1;
	b->ops->run(b, &s);
	i = 4;

	if (res[0] != 1)
		fatal("t2010:%i: bad 'trigger.high' for %li (%li)\n",
				i, input, res[0]);
	if (res[1] != 1)
		fatal("t2010:%i: bad 'trigger.low' for %li (%li)\n",
				i, input, res[1]);

	input = 250;
	res[0] = 0;
	res[1] = 1;
	b->ops->run(b, &s);
	i = 5;

	if (res[0] != 1)
		fatal("t2010:%i: bad 'trigger.high' for %li (%li)\n",
				i, input, res[0]);
	if (res[1] != 0)
		fatal("t2010:%i: bad 'trigger.low' for %li (%li)\n",
				i, input, res[1]);

	input = 50;
	res[0] = 1;
	res[1] = 0;
	b->ops->run(b, &s);
	i = 6;

	if (res[0] != 0)
		fatal("t2010:%i: bad 'trigger.high' for %li (%li)\n",
				i, input, res[0]);
	if (res[1] != 1)
		fatal("t2010:%i: bad 'trigger.low' for %li (%li)\n",
				i, input, res[1]);
	return 0;
}
