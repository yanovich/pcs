/* relay.c -- emulate relay block
 * Copyright (C) 2013 Sergey Yanovich <ynvich@gmail.com>
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

#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>

#include "includes.h"
#include "log.h"
#include "list.h"
#include "process.h"
#include "relay.h"

struct relay_config {
	int			block[256];
	int			block_count;
	int			unblock[256];
	int			unblock_count;
	int			no[256];
	int			no_count;
	int			nc[256];
	int			nc_count;
};

static void
relay_off(struct relay_config *c)
{
	int i;
	for (i = 0; i < c->no_count; i++)
		if (get_DO(c->no[i]))
			set_DO(c->no[i], 0, 0);
	for (i = 0; i < c->nc_count; i++)
		if (! get_DO(c->nc[i]))
			set_DO(c->nc[i], 1, 0);
}

static void
relay_on(struct relay_config *c)
{
	int i;
	debug2("relay on\n");
	for (i = 0; i < c->nc_count; i++)
		if (get_DO(c->nc[i]))
			set_DO(c->nc[i], 0, 0);
	for (i = 0; i < c->no_count; i++)
		if (! get_DO(c->no[i]))
			set_DO(c->no[i], 1, 0);
}

static void
relay_run(struct process *p, struct site_status *s)
{
	struct relay_config *c = p->config;
	int i;
	for (i = 0; i < c->block_count; i++) {
		if (! get_DI(c->block[i]))
			continue;
		relay_off(c);
		return;
	}
	for (i = 0; i < c->unblock_count; i++) {
		if (get_DI(c->unblock[i]))
			continue;
		relay_off(c);
		return;
	}
	relay_on(c);
}

static int
relay_log(struct site_status *s, void *conf, char *buff,
		const int sz, int o)
{
	struct relay_config *c = conf;
	int b = o;
	int i;

	if (o == sz)
		return 0;
	if (o) {
		buff[o] = ',';
		b++;
	}
	for (i = 0; (i < c->block_count) && (b < sz); i++, b++) {
		buff[b] = get_DI(c->block[i]) ? 'B' : '_';
	}
	if (i)
		buff[b++] = ' ';
	for (i = 0; (i < c->unblock_count) && (b < sz); i++, b++) {
		buff[b] = get_DI(c->unblock[i]) ? '_' : 'N';
	}
	if (i)
		buff[b++] = ' ';
	for (i = 0; (i < c->nc_count) && (b < sz); i++, b++) {
		buff[b] = get_DO(c->nc[i]) ? '-' : '_';
	}
	if (i)
		buff[b++] = ' ';
	for (i = 0; (i < c->no_count) && (b < sz); i++, b++) {
		buff[b] = get_DO(c->no[i]) ? '+' : '_';
	}
	buff[b] = 0;

	return b - o;
}

struct process_ops relay_ops = {
	.run = relay_run,
	.log = relay_log,
};

static void *
relay_init(void *conf)
{
	return &relay_ops;
}

struct setpoint_map relay_setpoints[] = {
	{
	}
};

static void
set_block(void *conf, int type, int value)
{
	struct relay_config *c = conf;
	if (type != DI_MODULE)
		fatal("relay: wrong type of block input\n");
	c->block[c->block_count++] = value;
	debug("  relay: block[%i] = %i\n", c->block_count - 1,
			c->block[c->block_count - 1]);
}

static void
set_unblock(void *conf, int type, int value)
{
	struct relay_config *c = conf;
	if (type != DI_MODULE)
		fatal("relay: wrong type of unblock input\n");
	c->unblock[c->unblock_count++] = value;
	debug("  relay: unblock[%i] = %i\n", c->unblock_count - 1,
			c->unblock[c->unblock_count - 1]);
}

static void
set_no(void *conf, int type, int value)
{
	struct relay_config *c = conf;
	if (type != DO_MODULE)
		fatal("relay: wrong type of NO output\n");
	c->no[c->no_count++] = value;
	debug("  relay: no[%i] = %i\n", c->no_count - 1,
			c->no[c->no_count - 1]);
}

static void
set_nc(void *conf, int type, int value)
{
	struct relay_config *c = conf;
	if (type != DO_MODULE)
		fatal("relay: wrong type of NC output\n");
	c->nc[c->nc_count++] = value;
	debug("  relay: nc[%i] = %i\n", c->nc_count - 1,
			c->nc[c->nc_count - 1]);
}

struct io_map relay_io[] = {
	{
		.name 		= "block",
		.set		= set_block,
	},
	{
		.name 		= "unblock",
		.set		= set_unblock,
	},
	{
		.name 		= "nc",
		.set		= set_nc,
	},
	{
		.name 		= "no",
		.set		= set_no,
	},
	{
	}
};

static void *
c_alloc(void)
{
	struct relay_config *c = xzalloc(sizeof(*c));
	return c;
}

struct process_builder relay_builder = {
	.alloc			= c_alloc,
	.setpoint		= relay_setpoints,
	.io			= relay_io,
	.ops			= relay_init,
};

struct process_builder *
load_relay_builder(void)
{
	return &relay_builder;
}
