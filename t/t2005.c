/* t/t2005.c -- test fuzzy logic S condition block
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
#include "fuzzy-if-s.h"
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

	bb = load_fuzzy_if_s_builder();
	b = xzalloc(sizeof(*b));
	b->data = bb->alloc();
	b->ops = bb->ops(b->data);
	if (!b->ops || !b->ops->run)
		fatal("t2005: bad 'fuzzy if s' ops\n");

	if (NULL == bb->inputs)
		fatal("t2005: bad 'fuzzy if s' input table\n");
	if (NULL != bb->inputs[0].key)
		fatal("t2005: bad 'fuzzy if s' input key\n");
	if (NULL == bb->inputs[0].value)
		fatal("t2005: bad 'fuzzy if s' input value\n");
	set_input = bb->inputs[0].value;
	set_input(b->data, "input", &input);

	if (NULL == bb->outputs)
		fatal("t2005: bad 'fuzzy if s' output table\n");
	if (NULL != bb->outputs[0])
		fatal("t2005: bad 'fuzzy if s' output count\n");
	b->outputs = &res;

	if (NULL == bb->setpoints)
		fatal("t2005: bad 'fuzzy if s' setpoints table\n");

	setter = pcs_lookup(bb->setpoints, "b");
	if (!setter)
		fatal("t2005: bad 'fuzzy if s' setpoint 'b'\n");
	setter(b->data, "b", -30);

	setter = pcs_lookup(bb->setpoints, "a");
	if (!setter)
		fatal("t2005: bad 'fuzzy if s' setpoint 'a'\n");
	setter(b->data, "a", -50);

	input = -60;
	res = 1;
	b->ops->run(b, &s);

	if (res != 0)
		fatal("t2005: bad 'fuzzy if s' error result for %li (%li)\n",
				input, res);

	input = -50;
	res = 1;
	b->ops->run(b, &s);

	if (res != 0)
		fatal("t2005: bad 'fuzzy if s' error result for %li (%li)\n",
				input, res);

	input = -40;
	res = 0;
	b->ops->run(b, &s);

	if (res != 0x8000)
		fatal("t2005: bad 'fuzzy if s' error result for %li (%li)\n",
				input, res);

	input = -30;
	res = 1;
	b->ops->run(b, &s);

	if (res != 0x10000)
		fatal("t2005: bad 'fuzzy if s' error result for %li (%li)\n",
				input, res);

	input = -20;
	res = 1;
	b->ops->run(b, &s);

	if (res != 0x10000)
		fatal("t2005: bad 'fuzzy if s' error result for %li (%li)\n",
				input, res);
	return 0;
}
