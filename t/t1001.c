/* t/t1001.c -- test file-input block
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
#include "file-input.h"
#include "list.h"
#include "map.h"
#include "serverconf.h"
#include "state.h"

static const char *nofile = "t/t1001.nofile";
static const char *good = "t/t1001.good";
static const char *bad = "t/t1001.bad";

int main(int argc, char **argv)
{
	struct server_state s = {
	};
	struct block_builder *bb;
	struct block *b;
	int (*setter_1)(void *, const char const *, long);
	int (*setter_3)(void *, const char const *, long);
	int (*setter_path)(void *, const char const *, const char const *);
	int (*setter_cache)(void *, const char const *, const char const *);
	long res[3];

	log_init("t1001", LOG_DEBUG + 2, LOG_DAEMON, 1);
	bb = load_file_input_builder();
	b = xzalloc(sizeof(*b));
	b->multiple = 1;
	b->data = bb->alloc();
	b->ops = bb->ops(b);
	if (b->ops)
		fatal("t1001: bad 'file-input' ops 1\n");

	if (NULL != bb->inputs)
		fatal("t1001: bad 'file-input' input table\n");

	if (NULL == bb->setpoints)
		fatal("t1001: bad 'file-input' setpoints table\n");

	if (NULL == bb->strings)
		fatal("t1001: bad 'file-input' strings table\n");

	setter_1 = pcs_lookup(bb->setpoints, "1");
	if (!setter_1)
		fatal("t1001: bad file-input setpoint '1'");
	setter_3 = pcs_lookup(bb->setpoints, "3");
	if (!setter_3)
		fatal("t1001: bad file-input setpoint '3'");

	setter_path = pcs_lookup(bb->strings, "path");
	if (!setter_path)
		fatal("t1001: bad 'file-input' string 'path'\n");
	setter_cache = pcs_lookup(bb->strings, "cache path");
	if (!setter_cache)
		fatal("t1001: bad 'file-input' string 'cache path'\n");

	setter_1(b->data, "1", 1);
	setter_3(b->data, "3", 3);
	setter_path(b->data, "path", nofile);

	b->ops = bb->ops(b);
	if (!b->ops || !b->ops->run)
		fatal("t1001: bad 'file-input' ops 2\n");

	if (NULL == b->outputs_table)
		fatal("t1001: bad 'file-input' output table\n");
	if (!b->outputs_table[0] || strcmp(b->outputs_table[0], "1"))
		fatal("t1001: bad 'file-input' output '1' %s\n",
				b->outputs_table[0]);
	if (!b->outputs_table[1] || strcmp(b->outputs_table[1], "3"))
		fatal("t1001: bad 'file-input' output '3' %s\n",
				b->outputs_table[1]);
	if (!b->outputs_table[2] || strcmp(b->outputs_table[2], "error"))
		fatal("t1001: bad 'file-input' output 'error' %s\n",
				b->outputs_table[2]);
	if (NULL != b->outputs_table[3])
		fatal("t1001: bad 'file-input' output count\n");
	b->outputs = res;

	res[0] = 0;
	res[1] = 0;
	res[2] = 0;
	b->ops->run(b, &s);

	if (res[0] != 1)
		fatal("t1001: bad '%li' '1' result for %s\n",
				res[0], nofile);
	if (res[1] != 3)
		fatal("t1001: bad '%li' '3' result for %s\n",
				res[1], nofile);
	if (res[2] != 1)
		fatal("t1001: bad '%li' error result for %s\n",
				res[2], nofile);

	xfree(b->outputs_table);
	xfree(b->data);

	b->data = bb->alloc();
	setter_1(b->data, "1", 1);
	setter_3(b->data, "3", 3);
	setter_path(b->data, "path", good);
	b->ops = bb->ops(b);
	if (!b->ops || !b->ops->run)
		fatal("t1001: bad 'file-input' ops 2\n");

	b->outputs = res;
	res[0] = 0;
	res[1] = 0;
	res[2] = 1;
	b->ops->run(b, &s);

	if (res[0] != 2)
		fatal("t1001: bad '%li' result 0 for %s\n",
				res[0], good);
	if (res[1] != 4)
		fatal("t1001: bad '%li' result 1 for %s\n",
				res[1], good);
	if (res[2] != 0)
		fatal("t1001: bad '%li' error result for %s\n",
				res[2], good);


	xfree(b->outputs_table);
	xfree(b->data);

	b->data = bb->alloc();
	setter_1(b->data, "1", 1);
	setter_3(b->data, "3", 3);
	setter_cache(b->data, "cache path", good);
	setter_path(b->data, "path", bad);
	b->ops = bb->ops(b);
	if (!b->ops || !b->ops->run)
		fatal("t1001: bad 'file-input' ops 3\n");

	b->outputs = res;
	res[0] = 0;
	res[1] = 0;
	res[2] = 0;
	b->ops->run(b, &s);

	if (res[0] != 2)
		fatal("t1001: bad '%li' result 0 for %s\n",
				res[0], bad);
	if (res[1] != 4)
		fatal("t1001: bad '%li' result 1 for %s\n",
				res[1], bad);
	if (res[2] != 1)
		fatal("t1001: bad '%li' error result for %s\n",
				res[2], bad);

	return 0;
}
