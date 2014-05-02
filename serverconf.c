/* serverconf.c -- load server configuration from a YAML file
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <yaml.h>

#include "block.h"
#include "i-8042.h"
#include "i-87015.h"
#include "ni1000tk5000.h"
#include "list.h"
#include "map.h"
#include "mark.h"
#include "serverconf.h"

const char *yaml_event_type[] = {
	"YAML_NO_EVENT",
	"YAML_STREAM_START_EVENT",
	"YAML_STREAM_END_EVENT",
	"YAML_DOCUMENT_START_EVENT",
	"YAML_DOCUMENT_END_EVENT",
	"YAML_ALIAS_EVENT",
	"YAML_SCALAR_EVENT",
	"YAML_SEQUENCE_START_EVENT",
	"YAML_SEQUENCE_END_EVENT",
	"YAML_MAPPING_START_EVENT",
	"YAML_MAPPING_END_EVENT"
};

struct pcs_parser_state {
	const char const	*filename;
	struct server_config	*conf;
};

struct pcs_parser_node {
	struct list_head		node_entry;
	struct pcs_parser_state		*state;
	int				(*handler[YAML_MAPPING_END_EVENT + 1])(
			struct pcs_parser_node *node, yaml_event_t *event);
	void				*data;
};

struct pcs_parser_map {
	const char		*key;
	int			(*handler)(
			struct pcs_parser_node *node, yaml_event_t *event);
	void			*data;
};

static int
handler_from_map(const char *key, struct pcs_parser_map *map)
{
	int i = 0;

	while (1) {
		if (!map[i].key || !strcmp(key, map[i].key))
			return i;
		i++;
		continue;
	}
}

static void
default_config(struct server_config *conf)
{
	conf->tick.tv_sec = 10;
	conf->tick.tv_usec = 0;
}

static struct pcs_parser_node*
new_parser_node(struct pcs_parser_state *state, struct list_head *parent,
		size_t extra)
{
	struct pcs_parser_node *n = xzalloc(sizeof(*n) + extra);
	if (!state || !parent)
		fatal("cannot create parser node\n");

	n->state = state;
	list_add(&n->node_entry, parent);
	if (extra)
		n->data = &n[1];

	return n;
}

static void
remove_parser_node(struct pcs_parser_node *node)
{
	list_del(&node->node_entry);
	xfree(node);
}

static int
unexpected_key(struct pcs_parser_node *node, yaml_event_t *event,
		const char *key)
{
	fatal("unexpected key %s in %s at line %zu column %zu\n",
			key,
			node->state->filename,
			event->start_mark.line,
			event->start_mark.column);
	return 0;
}

static int
unexpected_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	fatal("unexpected %s in %s at line %zu column %zu\n",
			yaml_event_type[event->type],
			node->state->filename,
			event->start_mark.line,
			event->start_mark.column);
	return 0;
}

static long
long_value(struct pcs_parser_node *node, yaml_event_t *event)
{
	char *bad;
	long val = strtol((const char *) event->data.scalar.value, &bad, 0);
	if (bad[0] != 0)
		fatal("bad integer(%s) in %s at line %zu column %zu\n",
				(const char *) event->data.scalar.value,
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);
	return val;
}

static int
end_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	remove_parser_node(node);
	return 1;
}

static int
exit_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	remove_parser_node(node);
	return 0;
}

static int
scalar_key_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct pcs_parser_node *n;
	struct pcs_parser_map *map = node->data;
	int (*handler)(struct pcs_parser_node *node, yaml_event_t *event);
	const char *key = (const char*) event->data.scalar.value;
	size_t len = event->data.scalar.length;
	int i;
	char *s;

	if (YAML_SCALAR_EVENT != event->type)
		return unexpected_event(node, event);

	if (len > 500)
		fatal("key (%s) is over 500 chars\n", key);

	len += sizeof(int *) - len % sizeof(int *);
	n = new_parser_node(node->state, &node->node_entry, len);
	s = (char *) &n[1];
	strncpy(s, key, len);

	i = handler_from_map(key, map);
	handler = map[i].handler;

	if (!handler)
		return unexpected_key(node, event, key);

	n->handler[YAML_SCALAR_EVENT] = handler;
	n->handler[YAML_SEQUENCE_START_EVENT] = handler;
	n->handler[YAML_MAPPING_START_EVENT] = handler;
	n->data = map[i].data;
	debug("%s:\n", key);
	return 1;
}

static int
map_start_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	if (YAML_MAPPING_START_EVENT != event->type)
		return unexpected_event(node, event);

	node->handler[YAML_SCALAR_EVENT] = scalar_key_event;
	node->handler[YAML_MAPPING_START_EVENT] = NULL;
	node->handler[YAML_MAPPING_END_EVENT] = end_event;
	return 1;
}

static int
map_sequence_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct pcs_parser_node *n = new_parser_node(node->state,
			&node->node_entry, 0);

	if (YAML_MAPPING_START_EVENT != event->type)
		return unexpected_event(node, event);

	n->handler[YAML_SCALAR_EVENT] = scalar_key_event;
	n->handler[YAML_MAPPING_END_EVENT] = end_event;
	n->data = node->data;
	return 1;
}

static int
options_tick_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	long msec;

	if (YAML_SCALAR_EVENT != event->type)
		return unexpected_event(node, event);

	msec = long_value(node, event);
	debug(" %li ms\n", msec);
	node->state->conf->tick.tv_sec = msec / 1000;
	node->state->conf->tick.tv_usec = (msec % 1000) * 1000;
	remove_parser_node(node);
	return 1;
}

static int
new_setpoint_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct block *b = list_entry(node->state->conf->block_list.prev,
			struct block, block_entry);
	const char *key = (const char *) &node[1];
	long value;
	int (*setter)(void *, long);

	if (YAML_SCALAR_EVENT != event->type)
		return unexpected_event(node, event);

	value = long_value(node, event);
	setter = pcs_lookup(b->builder->setpoints, key);
	if (!setter)
		return unexpected_key(node, event, key);

	debug(" %li\n", value);
	setter(b->data, value);
	remove_parser_node(node);
	return 1;
}

static int
block_input_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct block *b = list_entry(node->state->conf->block_list.prev,
			struct block, block_entry);
	const char *input = (const char *) event->data.scalar.value;

	if (YAML_SCALAR_EVENT != event->type)
		return unexpected_event(node, event);

	debug(" %s TODO: actually set input\n", input);
	remove_parser_node(node);
	return 1;
}

static int
block_name_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct block *b = list_entry(node->state->conf->block_list.prev,
			struct block, block_entry);
	const char *name = (const char *) event->data.scalar.value;

	if (YAML_SCALAR_EVENT != event->type)
		return unexpected_event(node, event);

	strncpy(b->name, name, PCS_MAX_NAME_LENGTH);
	b->name[PCS_MAX_NAME_LENGTH - 1] = 0;
	debug(" %s\n", name);
	remove_parser_node(node);
	return 1;
}

struct pcs_map loaders[] = {
	{
		.key		= "i-8042",
		.value		= load_i_8042_builder,
	}
	,{
		.key		= "i-87015",
		.value		= load_i_87015_builder,
	}
	,{
		.key		= "ni1000tk5000",
		.value		= load_ni1000tk5000_builder,
	}
	,{
		.key		= "mark",
		.value		= load_mark_builder,
	}
	,{
	}
};

static int
new_block_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	const char *key = (const char *) &node[1];
	struct block_builder *(*loader)(void) = NULL;
	struct block_builder *builder = NULL;
	struct block *b;

	if (YAML_MAPPING_START_EVENT != event->type)
		return unexpected_event(node, event);

	loader = pcs_lookup(loaders, key);
	if (!loader)
		return unexpected_key(node, event, (const char *) &node[1]);

	builder = loader();
	if (!builder)
		return unexpected_key(node, event, (const char *) &node[1]);

	b = xzalloc(sizeof(*b));
	b->builder = builder;
	if (builder->alloc)
		b->data = builder->alloc();

	b->ops = builder->ops();
	if (!b->ops || !b->ops->run)
		fatal("bad ops for block '%s'\n", key);

	list_add_tail(&b->block_entry, &node->state->conf->block_list);

	node->handler[YAML_SCALAR_EVENT] = NULL;
	node->handler[YAML_SEQUENCE_START_EVENT] = NULL;
	return map_start_event(node, event);
}

static int
blocks_start_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	if (YAML_SEQUENCE_START_EVENT != event->type)
		return unexpected_event(node, event);

	node->handler[YAML_SCALAR_EVENT] = NULL;
	node->handler[YAML_SEQUENCE_START_EVENT] = NULL;
	node->handler[YAML_MAPPING_START_EVENT] = map_sequence_event;
	node->handler[YAML_SEQUENCE_END_EVENT] = end_event;
	return 1;
}

struct pcs_parser_map options_map[] = {
	{
		.key			= "tick",
		.handler		= options_tick_event,
	}
	,{
	}
};

struct pcs_parser_map setpoints_map[] = {
	{
		.handler		= new_setpoint_event,
	}
	,{
	}
};

struct pcs_parser_map block_map[] = {
	{
		.key			= "input",
		.handler		= block_input_event,
	}
	,{
		.key			= "name",
		.handler		= block_name_event,
	}
	,{
		.key			= "setpoints",
		.handler		= map_start_event,
		.data			= &setpoints_map,
	}
	,{
	}
};

struct pcs_parser_map blocks_map[] = {
	{
		.handler		= new_block_event,
		.data			= &block_map,
	}
	,{
	}
};

struct pcs_parser_map top_map[] = {
	{
		.key			= "options",
		.handler		= map_start_event,
		.data			= &options_map,
	}
	,{
		.key			= "blocks",
		.handler		= blocks_start_event,
		.data			= &blocks_map,
	}
	,{
	}
};

static int
document_start_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	static int reentry;
	struct pcs_parser_node *n = new_parser_node(node->state,
			&node->node_entry, 0);

	if (reentry)
		fatal("multiple config documents not supported\n");

	node->handler[YAML_DOCUMENT_START_EVENT] = NULL;
	node->handler[YAML_DOCUMENT_END_EVENT] = end_event;
	n->handler[YAML_MAPPING_START_EVENT] = map_start_event;
	n->data = top_map;
	reentry = 1;
	return 1;
}

static int
stream_start_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct pcs_parser_node *n = new_parser_node(node->state,
			&node->node_entry, 0);
	node->handler[YAML_STREAM_START_EVENT] = NULL;
	node->handler[YAML_STREAM_END_EVENT] = exit_event;
	n->handler[YAML_DOCUMENT_START_EVENT] = document_start_event;
	return 1;
}

static void
parse_config(const char *filename, struct server_config *conf)
{
	int run = 1;
	FILE *f = fopen(filename, "r");
	yaml_parser_t parser;
	yaml_event_t event;
	int (*handler)(struct pcs_parser_node *node, yaml_event_t *event);
	struct pcs_parser_state state = {
		.filename = filename,
		.conf = conf,
	};
	LIST_HEAD(nodes);
	struct pcs_parser_node *node = new_parser_node(&state, &nodes, 0);

	if (NULL == f)
		fatal("failed to open %s (%s)\n", filename, strerror(errno));

	node->handler[YAML_STREAM_START_EVENT] = stream_start_event;
	yaml_parser_initialize(&parser);
	yaml_parser_set_input_file(&parser, f);

	while (run) {
		if (!yaml_parser_parse(&parser, &event))
			fatal("failed to parse %s\n", filename);
		node = list_entry(nodes.prev, struct pcs_parser_node,
				node_entry);
		debug3("%s (%p)\n", yaml_event_type[event.type], node);
		if (&node->node_entry == &nodes)
			fatal("empty parser list\n");
		handler = node->handler[event.type];
		if (!handler)
			handler = unexpected_event;
		run = handler(node, &event);
	}

	yaml_parser_delete(&parser);
	fclose(f);
}

void
load_server_config(const char const *filename, struct server_config *conf)
{
	default_config(conf);

	parse_config(filename, conf);
}
