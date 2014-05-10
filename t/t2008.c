/* t/t2008.c -- test discrete valve block
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
#include "discrete-valve.h"
#include "map.h"
#include "serverconf.h"
#include "state.h"

#define C_t2008_TICK		10
#define C_t2008_SPAN		100
#define C_t2008_INPUT_MULTIPLE	5

int main(int argc, char **argv)
{
	struct server_state s = {
		.tick		= { 0, C_t2008_TICK * 1000 },
	};
	struct block_builder *bb;
	struct block *b;
	void (*set_input)(void *, const char const *, long *);
	int (*setter)(void *, long);
	long input, res[4];
	int i = 1, stop;

	log_init("t2008", LOG_DEBUG + 2, LOG_DAEMON, 1);
	bb = load_discrete_valve_builder();
	b = xzalloc(sizeof(*b));
	b->multiple = 1;
	b->data = bb->alloc();
	b->ops = bb->ops(b->data);
	if (!b->ops || !b->ops->run)
		fatal("t2008: bad 'discrete valve' ops\n");

	if (NULL == bb->inputs)
		fatal("t2008: bad 'discrete valve' input table\n");
	if (NULL != bb->inputs[0].key)
		fatal("t2008: bad 'discrete valve' input key\n");
	if (NULL == bb->inputs[0].value)
		fatal("t2008: bad 'discrete valve' input value\n");
	set_input = bb->inputs[0].value;
	set_input(b->data, "input", &input);

	if (NULL == bb->outputs)
		fatal("t2008: bad 'discrete valve' output table\n");
	if (!bb->outputs[0] || strcmp(bb->outputs[0], "close"))
		fatal("t2008: bad 'discrete valve' output 'close'\n");
	if (!bb->outputs[1] || strcmp(bb->outputs[1], "open"))
		fatal("t2008: bad 'discrete valve' output 'open'\n");
	if (!bb->outputs[2] || strcmp(bb->outputs[2], "position"))
		fatal("t2008: bad 'discrete valve' output 'position'\n");
	if (!bb->outputs[3] || strcmp(bb->outputs[3], "absolute"))
		fatal("t2008: bad 'discrete valve' output 'absolute'\n");
	if (NULL != bb->outputs[4])
		fatal("t2008: bad 'discrete valve' output count\n");
	b->outputs = res;

	if (NULL == bb->setpoints)
		fatal("t2008: bad 'discrete valve' setpoints table\n");

	setter = pcs_lookup(bb->setpoints, "span");
	if (!setter)
		fatal("t2008: bad 'discrete valve' setpoint 'span'\n");
	setter(b->data, C_t2008_SPAN);

	setter = pcs_lookup(bb->setpoints, "input multiple");
	if (!setter)
		fatal("t2008: bad discrete-valve setpoint 'input multiple'\n");
	setter(b->data, C_t2008_INPUT_MULTIPLE);

	input = 0;
	res[0] = 1;
	res[1] = 1;
	res[2] = 0;
	res[3] = 0;
	b->ops->run(b, &s);
	{
		if (res[0] != 0)
			fatal("t2008: bad 'discrete valve' close "
					"for %li (%li) step 1-%i\n",
					input, res[0], i);
		if (res[1] != 0)
			fatal("t2008: bad 'discrete valve' open "
					"for %li (%li) step 1-%i\n",
					input, res[1], i);
		if (res[2] != 0)
			fatal("t2008: bad 'discrete valve' position "
					"for %li (%li) step 1-%i\n",
					input, res[2], i);
		if (res[3] != 0)
			fatal("t2008: bad 'discrete valve' absolute "
					"for %li (%li) step 1-%i\n",
					input, res[3], i);
	}


	input = C_t2008_TICK - 1;
	stop = C_t2008_INPUT_MULTIPLE + 1;
	for (i = 2; i <= stop; i++) {
		b->ops->run(b, &s);

		if (res[0] != 0)
			fatal("t2008: bad 'discrete valve' close "
					"for %li (%li) step 2-%i\n",
					input, res[0], i);
		if (res[1] != 0)
			fatal("t2008: bad 'discrete valve' open "
					"for %li (%li) step 2-%i\n",
					input, res[1], i);
	}

	input = C_t2008_TICK * C_t2008_INPUT_MULTIPLE;
	stop = C_t2008_INPUT_MULTIPLE * 2;
	for (; i <= stop; i++) {
		b->ops->run(b, &s);

		if (res[0] != 0)
			fatal("t2008: bad 'discrete valve' close "
					"for %li (%li) step 3-%i\n",
					input, res[0], i);
		if (res[1] != 0)
			fatal("t2008: bad 'discrete valve' open "
					"for %li (%li) step 3-%i\n",
					input, res[1], i);
	}

	stop += C_t2008_SPAN / C_t2008_TICK;
	res[0] = 1;
	res[1] = 0;
	for (; i <= stop; i++) {
		b->ops->run(b, &s);

		if (res[0] != 0)
			fatal("t2008: bad 'discrete valve' close "
					"for %li (%li) step 4-%i\n",
					input, res[0], i);
		if (res[1] != 1)
			fatal("t2008: bad 'discrete valve' open "
					"for %li (%li) step 4-%i\n",
					input, res[1], i);
	}
	{
		if (res[2] != C_t2008_SPAN / C_t2008_TICK)
			fatal("t2008: bad 'discrete valve' position "
					"for %li (%li) step 4-%i\n",
					input, res[2], i);
		if (res[3] != 1)
			fatal("t2008: bad 'discrete valve' absolute "
					"for %li (%li) step 4-%i\n",
					input, res[3], i);
	}

	stop += C_t2008_INPUT_MULTIPLE;
	res[0] = 1;
	res[1] = 1;
	for (; i <= stop; i++) {
		b->ops->run(b, &s);

		if (res[0] != 0)
			fatal("t2008: bad 'discrete valve' close "
					"for %li (%li) step 5-%i\n",
					input, res[0], i);
		if (res[1] != 0)
			fatal("t2008: bad 'discrete valve' open "
					"for %li (%li) step 5-%i\n",
					input, res[1], i);
	}

	input = -C_t2008_TICK * (C_t2008_INPUT_MULTIPLE - 1);
	stop = C_t2008_INPUT_MULTIPLE - 1;
	res[0] = 0;
	res[1] = 1;
	for (i = 1; i <= stop; i++) {
		b->ops->run(b, &s);

		if (res[0] != 1)
			fatal("t2008: bad 'discrete valve' close "
					"for %li (%li) step 6-%i\n",
					input, res[0], i);
		if (res[1] != 0)
			fatal("t2008: bad 'discrete valve' open "
					"for %li (%li) step 6-%i\n",
					input, res[1], i);
	}
	{
		b->ops->run(b, &s);

		if (res[0] != 0)
			fatal("t2008: bad 'discrete valve' close "
					"for %li (%li) step 6-%i\n",
					input, res[0], i);
		if (res[1] != 0)
			fatal("t2008: bad 'discrete valve' open "
					"for %li (%li) step 6-%i\n",
					input, res[1], i);
	}


	input = C_t2008_TICK * (C_t2008_INPUT_MULTIPLE - 2);
	stop = C_t2008_INPUT_MULTIPLE - 2;
	res[0] = 1;
	res[1] = 0;
	for (i = 1; i <= stop; i++) {
		b->ops->run(b, &s);

		if (res[0] != 0)
			fatal("t2008: bad 'discrete valve' close "
					"for %li (%li) step 7-%i\n",
					input, res[0], i);
		if (res[1] != 1)
			fatal("t2008: bad 'discrete valve' open "
					"for %li (%li) step 7-%i\n",
					input, res[1], i);
	}
	for (; i <= C_t2008_INPUT_MULTIPLE; i++) {
		b->ops->run(b, &s);

		if (res[0] != 0)
			fatal("t2008: bad 'discrete valve' close "
					"for %li (%li) step 7-%i\n",
					input, res[0], i);
		if (res[1] != 0)
			fatal("t2008: bad 'discrete valve' open "
					"for %li (%li) step 7-%i\n",
					input, res[1], i);
	}

	input = C_t2008_TICK * (2);
	res[0] = 0;
	res[1] = 1;
	for (i = 1; i <= C_t2008_INPUT_MULTIPLE; i++) {
		b->ops->run(b, &s);

		if (res[0] != 0)
			fatal("t2008: bad 'discrete valve' close "
					"for %li (%li) step 8-%i\n",
					input, res[0], i);
		if (res[1] != 1)
			fatal("t2008: bad 'discrete valve' open "
					"for %li (%li) step 8-%i\n",
					input, res[1], i);
	}

	return 0;
}
