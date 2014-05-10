/* logger.c -- block which logs system registers
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
#include <string.h>

#include "block.h"
#include "list.h"
#include "logger.h"
#include "map.h"
#include "state.h"

struct log_state {
	struct list_head		items;
};

struct log_item {
	struct list_head		item_entry;
	const char			*key;
	long				*value;
};

static void
log_run(struct block *b, struct server_state *s)
{
	const int sz = 1024;
	struct log_state *d = b->data;
	struct log_item *item;
	char buff[sz];
	int i = 0;

	list_for_each_entry(item, &d->items, item_entry) {
		i += snprintf(&buff[i], sz - i, "%s:%li ",
				item->key, *item->value);
		if (i >= (sz - 1))
			break;
	}
	buff[i] = 0;
	logit("%s\n", buff);
}

static struct block_ops ops = {
	.run		= log_run,
};

static struct block_ops *
init(void *data)
{
	return &ops;
}

static void *
alloc(void)
{
	struct log_state *d = xzalloc(sizeof(*d));
	INIT_LIST_HEAD(&d->items);
	return d;
}

static void
set_input(void *data, const char const *key, long *input)
{
	struct log_state *d = data;
	struct log_item *item = xzalloc(sizeof(*item));

	if (key)
		item->key = strdup(key);
	item->value = input;
	list_add_tail(&item->item_entry, &d->items);
}

static struct pcs_map inputs[] = {
	{
		.key			= NULL,
		.value			= set_input,
	}
};

static struct block_builder log_builder = {
	.ops		= init,
	.alloc		= alloc,
	.inputs		= inputs,
};

struct block_builder *
load_log_builder(void)
{
	return &log_builder;
}
