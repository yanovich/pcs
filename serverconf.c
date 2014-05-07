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
#include "block-list.h"
#include "list.h"
#include "map.h"
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
options_multiple_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	long mul;

	if (YAML_SCALAR_EVENT != event->type)
		return unexpected_event(node, event);

	mul = long_value(node, event);
	debug(" %li\n", mul);
	node->state->conf->multiple = mul;
	remove_parser_node(node);
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
	if (!b->data)
		fatal("trying to setup uninitialized block in "
				"%s line %zu column %zu\n",
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);
	if (setter(b->data, value))
		fatal("setpoint '%s' error in %s line %zu column %zu\n",
				key,
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);
	remove_parser_node(node);
	return 1;
}

static void
register_output(struct server_config *c, struct block *b)
{
	const char **outputs = b->builder->outputs;
	long *reg = &c->regs[c->regs_used];
	int i;

	if (!outputs)
		return;

	if (NULL == outputs[0]) {
		b->outputs = reg;
		c->regs_used++;
		if (c->regs_used == c->regs_count)
			fatal("%i registers are not enough\n", c->regs_count);
		return;
	}
	for (i = 0; outputs[i]; i++) {
		c->regs_used++;
		if (c->regs_used == c->regs_count)
			fatal("%i registers are not enough\n", c->regs_count);
	}
	b->outputs = reg;
}

static long *
find_output(struct list_head *list, const char const *key)
{
	struct block *b;
	const char **outputs;
	size_t len;
	int i;

	list_for_each_entry(b, list, block_entry) {
		outputs = b->builder->outputs;
		if (!outputs)
			continue;

		len = strlen(b->name);
		if (!len)
			continue;

		if (strncmp(key, b->name, len))
			continue;
		if (NULL == outputs[0]) {
			if (strlen(key) == len)
				return b->outputs;
			else
				continue;
		}
		if ('.' != key[len])
			continue;
		for (i = 0; outputs[i]; i++)
			if (!strcmp(outputs[i], &key[len + 1]))
				return &b->outputs[i];
	}
	return NULL;
}

static int
block_input_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct block *b = list_entry(node->state->conf->block_list.prev,
			struct block, block_entry);
	const char *input = (const char *) event->data.scalar.value;
	const char *key = (const char *) &node[1];
	void (*set_input)(void *, const char const *, long *);
	long *reg;

	if (YAML_SCALAR_EVENT != event->type)
		return unexpected_event(node, event);

	if (!b->builder->inputs)
		fatal("block has no inputs in %s at line %zu column %zu\n",
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);
	if (NULL != b->builder->inputs->key)
		fatal("ambiguos input in %s at line %zu column %zu\n",
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);
	reg = find_output(&node->state->conf->block_list, input);
	if (!reg)
		return unexpected_key(node, event, input);

	set_input = b->builder->inputs[0].value;
	set_input(b->data, key, reg);
	debug(" %s\n", input);
	remove_parser_node(node);
	return 1;
}

static int
block_multiple_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct block *b = list_entry(node->state->conf->block_list.prev,
			struct block, block_entry);
	const char *val = (const char *) event->data.scalar.value;
	char *bad;

	if (YAML_SCALAR_EVENT != event->type)
		return unexpected_event(node, event);

	b->multiple = (unsigned int) strtoul(val, &bad, 10);
	if (bad[0] != 0)
		fatal("bad integer (%s) in %s at line %zu column %zu\n",
				val,
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);
	debug(" %s\n", val);

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

static int
end_block_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct block *b = list_entry(node->state->conf->block_list.prev,
			struct block, block_entry);
	register_output(node->state->conf, b);
	return end_event(node, event);
}

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
	b->multiple = node->state->conf->multiple;
	b->counter = b->multiple;
	b->builder = builder;
	if (builder->alloc)
		b->data = builder->alloc();

	b->ops = builder->ops();
	if (!b->ops || !b->ops->run)
		fatal("bad ops for block '%s'\n", key);

	list_add_tail(&b->block_entry, &node->state->conf->block_list);

	node->handler[YAML_SCALAR_EVENT] = scalar_key_event;
	node->handler[YAML_MAPPING_START_EVENT] = NULL;
	node->handler[YAML_MAPPING_END_EVENT] = end_block_event;
	node->handler[YAML_SEQUENCE_START_EVENT] = NULL;
	return 1;
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
	if (!node->state->conf->regs_count)
		node->state->conf->regs_count = PCS_DEFAULT_REGS_COUNT;
	node->state->conf->regs = xzalloc(sizeof(*node->state->conf->regs) *
			node->state->conf->regs_count);
	return 1;
}

struct pcs_parser_map options_map[] = {
	{
		.key			= "multiple",
		.handler		= options_multiple_event,
	}
	,{
		.key			= "tick",
		.handler		= options_tick_event,
	}
	,{
	}
};

struct pcs_parser_map inputs_map[] = {
	{
		.handler		= block_input_event,
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
		.key			= "inputs",
		.handler		= map_start_event,
		.data			= &inputs_map,
	}
	,{
		.key			= "multiple",
		.handler		= block_multiple_event,
	}
	,{
		.key			= "name",
		.handler		= block_name_event,
	}
	,{
		.key			= "setpoint",
		.handler		= new_setpoint_event,
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
		if (YAML_SCALAR_EVENT != event.type)
			debug3("%s (%p)\n", yaml_event_type[event.type], node);
		else
			debug3("%s (%p) %s\n",
					yaml_event_type[event.type],
					node,
					event.data.scalar.value);
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
