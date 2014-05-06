/* t/t2006.c -- test fuzzy logic D value block
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
#include "fuzzy-then-d.h"
#include "map.h"
#include "serverconf.h"
#include "state.h"

int main(int argc, char **argv)
{
	struct server_state s;
	struct block_builder *bb;
	struct block *b;
	void (*set_input)(void *, const char const *, long *);
	int (*setter)(void *, long);
	long input, res[2];

	bb = load_fuzzy_then_d_builder();
	b = xzalloc(sizeof(*b));
	b->data = bb->alloc();
	b->ops = bb->ops();
	if (!b->ops || !b->ops->run)
		fatal("t2006: bad 'fuzzy then d' ops\n");

	if (NULL == bb->inputs)
		fatal("t2006: bad 'fuzzy then d' input table\n");
	if (NULL != bb->inputs[0].key)
		fatal("t2006: bad 'fuzzy then d' input key\n");
	if (NULL == bb->inputs[0].value)
		fatal("t2006: bad 'fuzzy then d' input value\n");
	set_input = bb->inputs[0].value;
	set_input(b->data, "input", &input);

	if (NULL == bb->outputs)
		fatal("t2006: bad 'fuzzy then d' output table\n");
	if (!bb->outputs[0] || strcmp(bb->outputs[0], "value"))
		fatal("t2006: bad 'fuzzy then d' output 'value'\n");
	if (!bb->outputs[1] || strcmp(bb->outputs[1], "weight"))
		fatal("t2006: bad 'fuzzy then d' output 'weight'\n");
	if (NULL != bb->outputs[2])
		fatal("t2006: bad 'fuzzy then d' output count\n");
	b->outputs = res;

	if (NULL == bb->setpoints)
		fatal("t2006: bad 'fuzzy then d' setpoints table\n");

	setter = pcs_lookup(bb->setpoints, "a");
	if (!setter)
		fatal("t2006: bad 'fuzzy then d' setpoint 'a'\n");
	setter(b->data, 100);

	setter = pcs_lookup(bb->setpoints, "b");
	if (!setter)
		fatal("t2006: bad 'fuzzy then d' setpoint 'b'\n");
	setter(b->data, 400);

	setter = pcs_lookup(bb->setpoints, "c");
	if (!setter)
		fatal("t2006: bad 'fuzzy then d' setpoint 'c'\n");
	setter(b->data, 700);

	input = 0;
	res[0] = 0;
	res[1] = 1;
	b->ops->run(b, &s);

	if (res[0] != 400)
		fatal("t2006: bad 'fuzzy then d' value for 0x%lx (%li)\n",
				input, res[0]);
	if (res[1] != 0)
		fatal("t2006: bad 'fuzzy then d' weight for 0x%lx (%li)\n",
				input, res[1]);

	input = 0x8000;
	res[0] = 0;
	res[1] = 0;
	b->ops->run(b, &s);

	if (res[0] != 400)
		fatal("t2006: bad 'fuzzy then d' value for 0x%lx (%li)\n",
				input, res[0]);
	if (res[1] != 900 * 0x40)
		fatal("t2006: bad 'fuzzy then d' weight for 0x%lx (%li)\n",
				input, res[1]);

	input = 0x10000;
	res[0] = 1;
	res[1] = 0;
	b->ops->run(b, &s);

	if (res[0] != 400)
		fatal("t2006: bad 'fuzzy then d' value for 0x%lx (%li)\n",
				input, res[0]);
	if (res[1] != 300 * 0x100)
		fatal("t2006: bad 'fuzzy then d' weight for 0x%lx (%li)\n",
				input, res[1]);
	return 0;
}
