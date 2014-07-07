/* t/t2011.c -- test logical not block
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
#include "logical-not.h"
#include "map.h"
#include "serverconf.h"
#include "state.h"

int main(int argc, char **argv)
{
	struct server_state s;
	struct block_builder *bb;
	struct block *b;
	void (*set_input)(void *, const char const *, long *);
	long input, res;

	bb = load_logical_not_builder();
	b = xzalloc(sizeof(*b));
	b->data = bb->alloc();

	if (NULL == bb->inputs)
		fatal("t2011: bad 'logical not' input table\n");
	if (NULL != bb->inputs[0].key)
		fatal("t2011: bad 'logical not' input key\n");
	if (NULL == bb->inputs[0].value)
		fatal("t2011: bad 'logical not' input value\n");
	set_input = bb->inputs[0].value;
	set_input(b->data, "input", &input);

	if (NULL == bb->outputs)
		fatal("t2011: bad 'logical not' output table\n");
	if (NULL != bb->outputs[0])
		fatal("t2011: bad 'logical not' output count\n");
	b->outputs = &res;

	if (NULL != bb->setpoints)
		fatal("t2011: bad 'logical not' setpoints table\n");

	b->ops = bb->ops(b);
	if (!b->ops || !b->ops->run)
		fatal("t2011: bad 'logical not' ops\n");

	input = 0;
	res = 0;
	b->ops->run(b, &s);

	if (res != 1)
		fatal("t2011: bad 'logical not' value 1 (%li)\n",
				res);

	input = 1;
	res = 1;
	b->ops->run(b, &s);

	if (res != 0)
		fatal("t2011: bad 'logical not' value 2 (%li)\n",
				res);

	input = 2;
	res = 1;
	b->ops->run(b, &s);

	if (res != 1)
		fatal("t2011: bad 'logical not' value 3 (%li)\n",
				res);

	input = 2;
	res = 0;
	b->ops->run(b, &s);

	if (res != 0)
		fatal("t2011: bad 'logical not' value 4 (%li)\n",
				res);

	return 0;
}
