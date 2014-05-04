/* t/t2002.c -- test Proportinal Differential block
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
#include "pd.h"
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

	bb = load_pd_builder();
	b = xzalloc(sizeof(*b));
	b->data = bb->alloc();
	b->ops = bb->ops();
	if (!b->ops || !b->ops->run)
		fatal("t2002: bad pd ops\n");

	if (NULL == bb->inputs)
		fatal("t2002: bad pd input table\n");
	if (NULL != bb->inputs[0].key)
		fatal("t2002: bad pd input key\n");
	if (NULL == bb->inputs[0].value)
		fatal("t2002: bad pd input value\n");
	set_input = bb->inputs[0].value;
	set_input(b->data, "input", &input);

	if (NULL == bb->outputs)
		fatal("t2002: bad pd output table\n");
	if (!bb->outputs[0] || strcmp(bb->outputs[0], "error"))
		fatal("t2002: bad pd output 'error'\n");
	if (!bb->outputs[1] || strcmp(bb->outputs[1], "diff"))
		fatal("t2002: bad pd output 'diff'\n");
	if (NULL != bb->outputs[2])
		fatal("t2002: bad pd output count\n");
	b->outputs = res;

	if (NULL == bb->setpoints)
		fatal("t2002: bad pd setpoints table\n");
	if (NULL != bb->setpoints[0].key)
		fatal("t2002: bad pd setpoint key\n");
	if (NULL == bb->setpoints[0].value)
		fatal("t2002: bad pd setpoint value\n");
	setter = bb->setpoints[0].value;
	setter(b->data, 1000);

	input = 1100;
	res[0] = 0;
	res[1] = 1;
	b->ops->run(b, &s);

	if (res[0] != 100)
		fatal("t2002: bad pd error result for %li (%li)\n",
				input, res[0]);
	if (res[1] != 0)
		fatal("t2002: bad initial pd diff (%li)\n",
				res[1]);

	input = 1200;
	res[0] = 0;
	res[1] = 1;
	b->ops->run(b, &s);

	if (res[0] != 200)
		fatal("t2002: bad pd error result for %li (%li)\n",
				input, res[0]);
	if (res[1] != 100)
		fatal("t2002: bad pd diff result for %li (%li)\n",
				input, res[1]);
	input = 1100;
	res[0] = 0;
	res[1] = 1;
	b->ops->run(b, &s);

	if (res[0] != 100)
		fatal("t2002: bad pd error result for %li (%li)\n",
				input, res[0]);
	if (res[1] != -100)
		fatal("t2002: bad pd diff result for %li (%li)\n",
				input, res[1]);
	return 0;
}
