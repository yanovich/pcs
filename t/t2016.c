/* t/t2016.c -- test linear converter block
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
#include "linear.h"
#include "list.h"
#include "map.h"
#include "serverconf.h"
#include "state.h"

int main(int argc, char **argv)
{
	struct server_state s;
	struct block_builder *bb;
	struct block *b;
	void (*set_input)(void *, const char const *, long *);
	int (*setter)(void *, const char const *, long);
	long input, res;

	bb = load_linear_builder();
	b = xzalloc(sizeof(*b));
	b->data = bb->alloc();
	b->ops = bb->ops(b);
	if (b->ops)
		fatal("t2016: bad 'linear' ops\n");

	if (NULL == bb->inputs)
		fatal("t2016: bad 'linear' input table\n");
	if (NULL != bb->inputs[0].key)
		fatal("t2016: bad 'linear' input key\n");
	if (NULL == bb->inputs[0].value)
		fatal("t2016: bad 'linear' input value\n");
	set_input = bb->inputs[0].value;
	set_input(b->data, "input", &input);

	if (NULL == bb->outputs)
		fatal("t2016: bad 'linear' output table\n");
	if (NULL != bb->outputs[0])
		fatal("t2016: bad 'linear' output count\n");
	b->outputs = &res;

	if (NULL == bb->setpoints)
		fatal("t2016: bad 'linear' setpoints table\n");

	setter = pcs_lookup(bb->setpoints, "in high");
	if (!setter)
		fatal("t2016: bad 'linear' setpoint 'in high'\n");
	setter(b->data, "in high", 20000);

	setter = pcs_lookup(bb->setpoints, "in low");
	if (!setter)
		fatal("t2016: bad 'linear' setpoint 'in low'\n");
	setter(b->data, "in low", 4000);

	setter = pcs_lookup(bb->setpoints, "out high");
	if (!setter)
		fatal("t2016: bad 'linear' setpoint 'out high'\n");
	setter(b->data, "out high", 1600);

	setter = pcs_lookup(bb->setpoints, "out low");
	if (!setter)
		fatal("t2016: bad 'linear' setpoint 'out low'\n");
	setter(b->data, "out low", 0);

	b->ops = bb->ops(b);
	if (!b->ops || !b->ops->run)
		fatal("t2016: bad 'linear' ops\n");

	input = 4000;
	res = 1;
	b->ops->run(b, &s);

	if (res != 0)
		fatal("t2016: bad 'linear' result 1 for %li (%li)\n",
				input, res);

	input = 20000;
	res = 1;
	b->ops->run(b, &s);

	if (res != 1600)
		fatal("t2016: bad 'linear' result 2 for %li (%li)\n",
				input, res);

	input = 12000;
	res = 1;
	b->ops->run(b, &s);

	if (res != 800)
		fatal("t2016: bad 'linear' result 3 for %li (%li)\n",
				input, res);
	return 0;
}
