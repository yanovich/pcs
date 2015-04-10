/* t/t2019.c -- test timer block
 * Copyright (C) 2015 Sergei Ianovich <s@asutp.io>
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
#include "timer.h"

int main(int argc, char **argv)
{
	struct server_state s;
	struct block_builder *bb;
	struct block *b;
	void (*set_input)(void *, const char const *, long *);
	int (*setter)(void *, const char const *, long);
	long input, res, i, stop = 3;

	log_init(__FILE__, LOG_DEBUG + 2, LOG_DAEMON, 1);
	bb = load_timer_builder();
	b = xzalloc(sizeof(*b));
	b->data = bb->alloc();

	if (NULL == bb->inputs)
		fatal(__FILE__ ": bad 'timer' input table\n");
	set_input = pcs_lookup(bb->inputs, "input");
	if (NULL == set_input)
		fatal(__FILE__ ": bad 'timer' input key\n");
	set_input(b->data, "input", &input);

	if (NULL == bb->outputs)
		fatal(__FILE__ ": bad 'timer' output table\n");
	if (NULL != bb->outputs[0])
		fatal(__FILE__ ": bad 'timer' output count\n");
	b->outputs = &res;

	if (NULL == bb->setpoints)
		fatal(__FILE__ ": bad 'timer' setpoints table\n");

	setter = pcs_lookup(bb->setpoints, "delay");
	if (!setter)
		fatal(__FILE__ ": bad 'timer' setpoint 'delay'\n");
	setter(b->data, "delay", stop);

	b->ops = bb->ops(b);
	if (!b->ops || !b->ops->run)
		fatal(__FILE__ ": bad 'timer' ops\n");

	input = 1;
	for (i = 0; i < stop; i++) {
		res = 1;
		b->ops->run(b, &s);

		if (res != 0)
			fatal(__FILE__ ": bad 'timer' result 1.%i for %li (%li)\n",
					i, input, res);
	}

	for (i = 0; i < 2; i++) {
		res = 0;
		b->ops->run(b, &s);

		if (res != 1)
			fatal(__FILE__ ": bad 'timer' result 2.%i for %li (%li)\n",
					i, input, res);
	}

	input = 0;
	{
		res = 1;
		b->ops->run(b, &s);

		if (res != 0)
			fatal(__FILE__ ": bad 'timer' result 3.%i for %li (%li)\n",
					i, input, res);
	}

	input = 1;
	for (i = 0; i < stop; i++) {
		res = 1;
		b->ops->run(b, &s);

		if (res != 0)
			fatal(__FILE__ ": bad 'timer' result 4.%i for %li (%li)\n",
					i, input, res);
	}

	for (i = 0; i < 2; i++) {
		res = 0;
		b->ops->run(b, &s);

		if (res != 1)
			fatal(__FILE__ ": bad 'timer' result 5.%i for %li (%li)\n",
					i, input, res);
	}

	return 0;
}
