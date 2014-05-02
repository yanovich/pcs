/* ni1000tk5000.c -- convert Ohms to grads C
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

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "block.h"
#include "ni1000tk5000.h"
#include "map.h"
#include "state.h"

#define PCS_BLOCK	"ni1000tk5000"

struct ni1000tk5000_state {
	unsigned		slot;
};

static void
ni1000tk5000_run(struct block *b, struct server_state *s)
{
	char buff[24];
	struct tm tm = *localtime(&s->start.tv_sec);
	struct ni1000tk5000_state *d = b->data;
	long result = 0;
	int err;

	strftime(&buff[0], sizeof(buff) - 1, "%b %e %H:%M:%S", &tm);
	debug("%s %s: ni1000tk5000 %li\n",
			buff, b->name, result);
}

static void *
ni1000tk5000_alloc(void)
{
	return xzalloc(sizeof(struct ni1000tk5000_state));
}

static struct block_ops ni1000tk5000_ops = {
	.run		= ni1000tk5000_run,
};

static struct block_ops *
ni1000tk5000_init(void)
{
	return &ni1000tk5000_ops;
}

static struct block_builder ni1000tk5000_builder = {
	.alloc		= ni1000tk5000_alloc,
	.ops		= ni1000tk5000_init,
};

struct block_builder *
load_ni1000tk5000_builder(void)
{
	return &ni1000tk5000_builder;
}
