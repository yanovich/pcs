/* pcs.c -- process control service
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

#include <sys/time.h>
#include <unistd.h>

#include "block.h"
#include "list.h"
#include "ni1000tk5000.h"
#include "map.h"
#include "serverconf.h"
#include "state.h"

int main(int argc, char **argv)
{
	struct server_state s;
	struct block_builder *bb;
	struct block *b;
	void (*set_input)(void *, const char const *, long *);
	long ai, res;

	bb = load_ni1000tk5000_builder();
	b = xzalloc(sizeof(*b));
	b->data = bb->alloc();
	b->ops = bb->ops();
	if (!b->ops || !b->ops->run)
		fatal("t2001: bad ni1000tk5000 ops\n");

	if (bb->inputs[0].key)
		fatal("t2001: bad ni1000tk5000 input key\n");

	if (NULL != bb->outputs[0])
		fatal("t2001: bad ni1000tk5000 output count\n");

	set_input = bb->inputs[0].value;
	set_input(b->data, "input", &ai);
	b->outputs = &res;

	ai = 10000;
	res = -2000;
	b->ops->run(b, &s);

	if (res != 0)
		fatal("t2001: bad ni1000tk5000 result for 1000 Ohms (%li)\n",
				res);

	ai = 15000;
	res = -2000;
	b->ops->run(b, &s);

	if (res != 1000)
		fatal("t2001: bad ni1000tk5000 result for 1500 Ohms (%li)\n",
				res);

	return 0;
}
