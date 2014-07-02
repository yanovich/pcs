/* t/t2018.c -- test r404a convereter
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
#include "r404a.h"
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

	bb = load_r404a_builder();
	b = xzalloc(sizeof(*b));
	b->data = bb->alloc();
	b->ops = bb->ops(b->data);
	if (!b->ops || !b->ops->run)
		fatal("t2017: bad r404a ops\n");

	if (bb->inputs[0].key)
		fatal("t2017: bad r404a input key\n");

	if (NULL != bb->outputs[0])
		fatal("t2017: bad r404a output count\n");

	set_input = bb->inputs[0].value;
	set_input(b->data, "input", &ai);
	b->outputs = &res;

	ai = 5970;
	res = -2000;
	b->ops->run(b, &s);

	if (res != 0)
		fatal("t2017: bad r404a result for 5970 mbars (%li)\n",
				res);

	ai = 16000;
	res = -2000;
	b->ops->run(b, &s);

	if (res != 350)
		fatal("t2017: bad r404a result for 16000 mbars (%li)\n",
				res);

	ai = 1309;
	res = -2000;
	b->ops->run(b, &s);

	if (res != -400)
		fatal("t2017: bad r404a result for 1309 mbars (%li)\n",
				res);

	return 0;
}
