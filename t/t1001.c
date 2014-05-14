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
	int (*setter_key)(void *, const char const *, const char const *);
	int (*setter_path)(void *, const char const *, const char const *);
	long res[3];

	log_init("t1001", LOG_DEBUG + 2, LOG_DAEMON, 1);
	bb = load_file_input_builder();
	b = xzalloc(sizeof(*b));
	b->multiple = 1;
	b->data = bb->alloc();
	b->ops = bb->ops(b->data);
	if (b->ops)
		fatal("t1001: bad 'file-input' ops 1\n");

	if (NULL != bb->inputs)
		fatal("t1001: bad 'file-input' input table\n");

	if (NULL != bb->setpoints)
		fatal("t1001: bad 'file-input' setpoints table\n");

	if (NULL == bb->strings)
		fatal("t1001: bad 'file-input' strings table\n");

	setter_key = pcs_lookup(bb->strings, "key");
	if (!setter_key)
		fatal("t1001: bad file-input string 'key");
	setter_key(b->data, "key", "1");
	setter_key(b->data, "key", "3");

	setter_path = pcs_lookup(bb->strings, "path");
	if (!setter_path)
		fatal("t1001: bad 'file-input' string 'path'\n");
	setter_path(b->data, "path", nofile);

	b->ops = bb->ops(b->data);
	if (!b->ops || !b->ops->run)
		fatal("t1001: bad 'file-input' ops 2\n");

	if (NULL == bb->outputs)
		fatal("t1001: bad 'file-input' output table\n");
	if (!bb->outputs[0] || strcmp(bb->outputs[0], "1"))
		fatal("t1001: bad 'file-input' output '1' %s\n",
				bb->outputs[0]);
	if (!bb->outputs[1] || strcmp(bb->outputs[1], "3"))
		fatal("t1001: bad 'file-input' output '3' %s\n",
				bb->outputs[1]);
	if (!bb->outputs[2] || strcmp(bb->outputs[2], "error"))
		fatal("t1001: bad 'file-input' output 'error' %s\n",
				bb->outputs[2]);
	if (NULL != bb->outputs[3])
		fatal("t1001: bad 'file-input' output count\n");
	b->outputs = res;

	res[0] = 0;
	res[1] = 0;
	res[2] = 0;
	b->ops->run(b, &s);

	if (res[2] != 1)
		fatal("t1001: bad '%li' error result for %s\n",
				res[2], nofile);

	xfree(bb->outputs);
	xfree(b->data);
	b->data = bb->alloc();
	setter_path(b->data, "path", good);
	setter_key(b->data, "key", "1");
	setter_key(b->data, "key", "3");
	b->ops = bb->ops(b->data);
	if (!b->ops || !b->ops->run)
		fatal("t1001: bad 'file-input' ops 3\n");

	b->outputs = res;
	res[0] = 1;
	res[1] = 0;
	res[2] = 1;
	b->ops->run(b, &s);

	if (res[0] != 0)
		fatal("t1001: bad '%li' result 0 for %s\n",
				res[0], good);
	if (res[1] != 4)
		fatal("t1001: bad '%li' result 1 for %s\n",
				res[1], good);
	if (res[2] != 0)
		fatal("t1001: bad '%li' error result for %s\n",
				res[2], good);


	xfree(bb->outputs);
	xfree(b->data);
	b->data = bb->alloc();
	setter_path(b->data, "path", bad);
	setter_key(b->data, "key", "1");
	setter_key(b->data, "key", "3");
	b->ops = bb->ops(b->data);
	if (!b->ops || !b->ops->run)
		fatal("t1001: bad 'file-input' ops 3\n");

	b->outputs = res;
	res[0] = 1;
	res[1] = 0;
	res[2] = 0;
	b->ops->run(b, &s);

	if (res[0] != 0)
		fatal("t1001: bad '%li' result 0 for %s\n",
				res[0], bad);
	if (res[1] != 0)
		fatal("t1001: bad '%li' result 1 for %s\n",
				res[1], bad);
	if (res[2] != 1)
		fatal("t1001: bad '%li' error result for %s\n",
				res[2], bad);

	return 0;
}
