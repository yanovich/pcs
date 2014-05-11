/* t/t2015.c -- test cascade block
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
#include "cascade.h"
#include "map.h"
#include "serverconf.h"
#include "state.h"

#define C_t2015_OUTPUTS		4
#define C_t2015_STAGE		5
#define C_t2015_UNSTAGE		10

int main(int argc, char **argv)
{
	struct server_state s = {
	};
	struct block_builder *bb;
	struct block *b;
	void (*set_input)(void *, const char const *, long *);
	int (*setter)(void *, long);
	long stage, unstage, stop, res[C_t2015_OUTPUTS];
	int i = 1;
	int j;

	log_init("t2015", LOG_DEBUG + 2, LOG_DAEMON, 1);
	bb = load_cascade_builder();
	b = xzalloc(sizeof(*b));
	b->multiple = 1;
	b->data = bb->alloc();
	b->ops = bb->ops(b->data);
	if (b->ops)
		fatal("t2015: bad 'cascade' ops 1\n");

	if (NULL == bb->inputs)
		fatal("t2015: bad 'cascade' input table\n");
	set_input = pcs_lookup(bb->inputs, "stage");
	if (NULL == set_input)
		fatal("t2015: bad 'cascade.stage' input\n");
	set_input(b->data, "stage", &stage);

	set_input = pcs_lookup(bb->inputs, "unstage");
	if (NULL == set_input)
		fatal("t2015: bad 'cascade.unstage' input\n");
	set_input(b->data, "unstage", &unstage);

	set_input = pcs_lookup(bb->inputs, "stop");
	if (NULL == set_input)
		fatal("t2015: bad 'cascade.stop' input\n");
	set_input(b->data, "stop", &stop);

	if (NULL != bb->inputs[3].key)
		fatal("t2015: bad 'cascade' input count\n");

	if (NULL == bb->setpoints)
		fatal("t2015: bad 'cascade' setpoints table\n");

	setter = pcs_lookup(bb->setpoints, "output count");
	if (!setter)
		fatal("t2015: bad 'cascade' setpoint 'output count'\n");
	setter(b->data, C_t2015_OUTPUTS);

	setter = pcs_lookup(bb->setpoints, "stage interval");
	if (!setter)
		fatal("t2015: bad cascade setpoint 'stage interval'\n");
	setter(b->data, C_t2015_STAGE);

	setter = pcs_lookup(bb->setpoints, "unstage interval");
	if (!setter)
		fatal("t2015: bad cascade setpoint 'unstage interval'\n");
	setter(b->data, C_t2015_UNSTAGE);

	b->ops = bb->ops(b->data);
	if (!b->ops || !b->ops->run)
		fatal("t2015: bad 'cascade' ops 2\n");

	if (NULL == bb->outputs)
		fatal("t2015: bad 'cascade' output table\n");
	if (!bb->outputs[0] || strcmp(bb->outputs[0], "1"))
		fatal("t2015: bad 'cascade' output '1' %s\n", bb->outputs[0]);
	if (!bb->outputs[1] || strcmp(bb->outputs[1], "2"))
		fatal("t2015: bad 'cascade' output '2'\n");
	if (!bb->outputs[2] || strcmp(bb->outputs[2], "3"))
		fatal("t2015: bad 'cascade' output '3'\n");
	if (!bb->outputs[3] || strcmp(bb->outputs[3], "4"))
		fatal("t2015: bad 'cascade' output '4'\n");
	if (NULL != bb->outputs[4])
		fatal("t2015: bad 'cascade' output count\n");
	b->outputs = res;

	stage = 1;
	unstage = 0;
	stop = 1;
	res[0] = 1;
	res[1] = 1;
	res[2] = 1;
	res[3] = 1;
	b->ops->run(b, &s);
	{
		if (res[0] != 0)
			fatal("t2015: bad 'cascade' m1 "
					"(%li) for step 1-%i\n",
					res[0], i);
		if (res[1] != 0)
			fatal("t2015: bad 'cascade' m2 "
					"(%li) for step 1-%i\n",
					res[1], i);
		if (res[2] != 0)
			fatal("t2015: bad 'cascade' m3 "
					"(%li) for step 1-%i\n",
					res[2], i);
		if (res[3] != 0)
			fatal("t2015: bad 'cascade' m4 "
					"(%li) for step 1-%i\n",
					res[3], i);
	}

	stage = 1;
	unstage = 0;
	stop = 0;
	b->ops->run(b, &s);
	for (j = 0; j < 4; j++)
		if (res[j] == 1)
			break;
	if (4 == j)
		fatal("t2015: unexpected state at step 2\n");
	{
		if (res[(j + 0) %4] != 1)
			fatal("t2015: bad 'cascade' m1 "
					"(%li) for step 2-%i\n",
					res[(j + 0) %4], i);
		if (res[(j + 1) %4] != 0)
			fatal("t2015: bad 'cascade' m2 "
					"(%li) for step 2-%i\n",
					res[(j + 1) %4], i);
		if (res[(j + 2) %4] != 0)
			fatal("t2015: bad 'cascade' m3 "
					"(%li) for step 2-%i\n",
					res[(j + 2) %4], i);
		if (res[(j + 3) %4] != 0)
			fatal("t2015: bad 'cascade' m4 "
					"(%li) for step 2-%i\n",
					res[(j + 3) %4], i);
	}

	stage = 0;
	unstage = 1;
	stop = 0;
	for (i = 1; i <= C_t2015_STAGE - 2; i++) {
		b->ops->run(b, &s);
		if (res[(j + 0) %4] != 1)
			fatal("t2015: bad 'cascade' m1 "
					"(%li) for step 3-%i\n",
					res[(j + 0) %4], i);
		if (res[(j + 1) %4] != 0)
			fatal("t2015: bad 'cascade' m2 "
					"(%li) for step 3-%i\n",
					res[(j + 1) %4], i);
		if (res[(j + 2) %4] != 0)
			fatal("t2015: bad 'cascade' m3 "
					"(%li) for step 3-%i\n",
					res[(j + 2) %4], i);
		if (res[(j + 3) %4] != 0)
			fatal("t2015: bad 'cascade' m4 "
					"(%li) for step 3-%i\n",
					res[(j + 3) %4], i);
	}

	stage = 1;
	unstage = 0;
	stop = 0;
	b->ops->run(b, &s);
	{
		if (res[(j + 0) %4] != 1)
			fatal("t2015: bad 'cascade' m1 "
					"(%li) for step 4-%i\n",
					res[(j + 0) %4], i);
		if (res[(j + 1) %4] != 0)
			fatal("t2015: bad 'cascade' m2 "
					"(%li) for step 4-%i\n",
					res[(j + 1) %4], i);
		if (res[(j + 2) %4] != 0)
			fatal("t2015: bad 'cascade' m3 "
					"(%li) for step 4-%i\n",
					res[(j + 2) %4], i);
		if (res[(j + 3) %4] != 0)
			fatal("t2015: bad 'cascade' m4 "
					"(%li) for step 4-%i\n",
					res[(j + 3) %4], i);
	}
	i++;
	b->ops->run(b, &s);
	{
		if (res[(j + 0) %4] != 1)
			fatal("t2015: bad 'cascade' m1 "
					"(%li) for step 5-%i\n",
					res[(j + 0) %4], i);
		if (res[(j + 1) %4] != 1)
			fatal("t2015: bad 'cascade' m2 "
					"(%li) for step 5-%i\n",
					res[(j + 1) %4], i);
		if (res[(j + 2) %4] != 0)
			fatal("t2015: bad 'cascade' m3 "
					"(%li) for step 5-%i\n",
					res[(j + 2) %4], i);
		if (res[(j + 3) %4] != 0)
			fatal("t2015: bad 'cascade' m4 "
					"(%li) for step 5-%i\n",
					res[(j + 3) %4], i);
	}
	i++;

	stage = 0;
	unstage = 1;
	stop = 0;
	for (; i <= C_t2015_UNSTAGE - 2; i++) {
		b->ops->run(b, &s);
		if (res[(j + 0) %4] != 1)
			fatal("t2015: bad 'cascade' m1 "
					"(%li) for step 6-%i\n",
					res[(j + 0) %4], i);
		if (res[(j + 1) %4] != 1)
			fatal("t2015: bad 'cascade' m2 "
					"(%li) for step 6-%i\n",
					res[(j + 1) %4], i);
		if (res[(j + 2) %4] != 0)
			fatal("t2015: bad 'cascade' m3 "
					"(%li) for step 6-%i\n",
					res[(j + 2) %4], i);
		if (res[(j + 3) %4] != 0)
			fatal("t2015: bad 'cascade' m4 "
					"(%li) for step 6-%i\n",
					res[(j + 3) %4], i);
	}
	{
		b->ops->run(b, &s);
		if (res[(j + 0) %4] != 0)
			fatal("t2015: bad 'cascade' m1 "
					"(%li) for step 7-%i\n",
					res[(j + 0) %4], i);
		if (res[(j + 1) %4] != 1)
			fatal("t2015: bad 'cascade' m2 "
					"(%li) for step 7-%i\n",
					res[(j + 1) %4], i);
		if (res[(j + 2) %4] != 0)
			fatal("t2015: bad 'cascade' m3 "
					"(%li) for step 7-%i\n",
					res[(j + 2) %4], i);
		if (res[(j + 3) %4] != 0)
			fatal("t2015: bad 'cascade' m4 "
					"(%li) for step 7-%i\n",
					res[(j + 3) %4], i);
	}

	stage = 1;
	unstage = 0;
	stop = 0;
	for (i = 1; i <= C_t2015_STAGE; i++) {
		b->ops->run(b, &s);
		if (res[(j + 0) %4] != 0)
			fatal("t2015: bad 'cascade' m1 "
					"(%li) for step 8-%i\n",
					res[(j + 0) %4], i);
		if (res[(j + 1) %4] != 1)
			fatal("t2015: bad 'cascade' m2 "
					"(%li) for step 8-%i\n",
					res[(j + 1) %4], i);
		if (res[(j + 2) %4] != 1)
			fatal("t2015: bad 'cascade' m3 "
					"(%li) for step 8-%i\n",
					res[(j + 2) %4], i);
		if (res[(j + 3) %4] != 0)
			fatal("t2015: bad 'cascade' m4 "
					"(%li) for step 8-%i\n",
					res[(j + 3) %4], i);
	}
	for (i = 1; i <= C_t2015_STAGE; i++) {
		b->ops->run(b, &s);
		if (res[(j + 0) %4] != 0)
			fatal("t2015: bad 'cascade' m1 "
					"(%li) for step 9-%i\n",
					res[(j + 0) %4], i);
		if (res[(j + 1) %4] != 1)
			fatal("t2015: bad 'cascade' m2 "
					"(%li) for step 9-%i\n",
					res[(j + 1) %4], i);
		if (res[(j + 2) %4] != 1)
			fatal("t2015: bad 'cascade' m3 "
					"(%li) for step 9-%i\n",
					res[(j + 2) %4], i);
		if (res[(j + 3) %4] != 1)
			fatal("t2015: bad 'cascade' m4 "
					"(%li) for step 9-%i\n",
					res[(j + 3) %4], i);
	}
	i = 1;
	{
		b->ops->run(b, &s);
		if (res[(j + 0) %4] != 1)
			fatal("t2015: bad 'cascade' m1 "
					"(%li) for step 10-%i\n",
					res[(j + 0) %4], i);
		if (res[(j + 1) %4] != 1)
			fatal("t2015: bad 'cascade' m2 "
					"(%li) for step 10-%i\n",
					res[(j + 1) %4], i);
		if (res[(j + 2) %4] != 1)
			fatal("t2015: bad 'cascade' m3 "
					"(%li) for step 10-%i\n",
					res[(j + 2) %4], i);
		if (res[(j + 3) %4] != 1)
			fatal("t2015: bad 'cascade' m4 "
					"(%li) for step 10-%i\n",
					res[(j + 3) %4], i);
	}

	stage = 0;
	unstage = 1;
	stop = 0;
	{
		b->ops->run(b, &s);
		if (res[(j + 0) %4] != 1)
			fatal("t2015: bad 'cascade' m1 "
					"(%li) for step 11-%i\n",
					res[(j + 0) %4], i);
		if (res[(j + 1) %4] != 0)
			fatal("t2015: bad 'cascade' m2 "
					"(%li) for step 11-%i\n",
					res[(j + 1) %4], i);
		if (res[(j + 2) %4] != 1)
			fatal("t2015: bad 'cascade' m3 "
					"(%li) for step 11-%i\n",
					res[(j + 2) %4], i);
		if (res[(j + 3) %4] != 1)
			fatal("t2015: bad 'cascade' m4 "
					"(%li) for step 11-%i\n",
					res[(j + 3) %4], i);
	}
	return 0;
}
