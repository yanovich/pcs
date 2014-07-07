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
#include "pcs-parser.h"
#include "serverconf.h"

static void
default_config(struct server_config *conf)
{
	conf->state.tick.tv_sec = 10;
	conf->state.tick.tv_usec = 0;
}

static int
map_sequence_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct pcs_parser_node *n = pcs_parser_new_node(node->state,
			&node->node_entry, 0);

	if (!n)
		fatal("%s: empty node\n", __FUNCTION__);

	if (YAML_MAPPING_START_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	n->handler[YAML_SCALAR_EVENT] = pcs_parser_scalar_key;
	n->handler[YAML_MAPPING_END_EVENT] = pcs_parser_up;
	n->data = node->data;
	return 1;
}

static int
options_multiple_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	long mul;
	struct server_config *conf = node->state->data;

	if (YAML_SCALAR_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	mul = pcs_parser_long(node, event, NULL);
	debug(" %li\n", mul);
	conf->multiple = mul;
	pcs_parser_remove_node(node);
	return 1;
}

static int
options_tick_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	long msec;
	struct server_config *conf = node->state->data;

	if (YAML_SCALAR_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	msec = pcs_parser_long(node, event, NULL);
	debug(" %li ms\n", msec);
	conf->state.tick.tv_sec = msec / 1000;
	conf->state.tick.tv_usec = (msec % 1000) * 1000;
	pcs_parser_remove_node(node);
	return 1;
}

static int
new_setpoint_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct server_config *conf = node->state->data;
	struct block *b = list_entry(conf->block_list.prev,
			struct block, block_entry);
	const char *key = (const char *) &node[1];
	long value;
	int (*setter)(void *, long);

	if (YAML_SCALAR_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	value = pcs_parser_long(node, event, NULL);
	setter = pcs_lookup(b->builder->setpoints, key);
	if (!setter)
		return pcs_parser_unexpected_key(node, event, key);

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
	pcs_parser_remove_node(node);
	return 1;
}

static int
new_string_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct server_config *conf = node->state->data;
	struct block *b = list_entry(conf->block_list.prev,
			struct block, block_entry);
	const char *key = (const char *) &node[1];
	const char *value = (const char *) event->data.scalar.value;
	int (*setter)(void *, const char *, const char *);

	if (0 == node->sequence && YAML_SEQUENCE_START_EVENT == event->type) {
		node->handler[YAML_SEQUENCE_END_EVENT] = pcs_parser_up;
		node->sequence = 1;
		return 1;
	} else if (YAML_SCALAR_EVENT != event->type) {
		return pcs_parser_unexpected_event(node, event);
	} else if (0 == node->sequence) {
		node->sequence = -1;
	}

	setter = pcs_lookup(b->builder->strings, key);
	if (!setter)
		return pcs_parser_unexpected_key(node, event, key);

	debug(" %s\n", value);
	if (!b->data)
		fatal("trying to setup uninitialized block in "
				"%s line %zu column %zu\n",
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);
	if (setter(b->data, key, value))
		fatal("string '%s' error in %s line %zu column %zu\n",
				key,
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);
	if (1 != node->sequence)
		pcs_parser_remove_node(node);
	return 1;
}

static void
register_output(struct server_config *c, struct block *b)
{
	const char **outputs = b->outputs_table;
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
	debug3(" %s: %i outputs\n", b->name, i);
}

static long *
find_output(struct list_head *list, const char const *key)
{
	struct block *b;
	const char **outputs;
	size_t len;
	int i;

	list_for_each_entry(b, list, block_entry) {
		outputs = b->outputs_table;
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
	struct server_config *conf = node->state->data;
	struct block *b = list_entry(conf->block_list.prev,
			struct block, block_entry);
	const char *input = (const char *) event->data.scalar.value;
	const char *key = (const char *) &node[1];
	void (*set_input)(void *, const char const *, long *);
	long *reg;

	if (0 == node->sequence && YAML_SEQUENCE_START_EVENT == event->type) {
		node->handler[YAML_SEQUENCE_END_EVENT] = pcs_parser_up;
		node->sequence = 1;
		return 1;
	} else if (YAML_SCALAR_EVENT != event->type) {
		return pcs_parser_unexpected_event(node, event);
	} else if (0 == node->sequence) {
		node->sequence = -1;
	}

	if (!b->builder->inputs)
		fatal("block has no inputs in %s at line %zu column %zu\n",
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);

	set_input = pcs_lookup(b->builder->inputs, key);
	if (NULL == set_input)
		fatal("ambiguos input in %s at line %zu column %zu\n",
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);
	reg = find_output(&conf->block_list, input);
	if (!reg)
		return pcs_parser_unexpected_key(node, event, input);

	set_input(b->data, key, reg);
	debug(" %s\n", input);
	if (1 != node->sequence)
		pcs_parser_remove_node(node);
	return 1;
}

static int
block_multiple_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct server_config *conf = node->state->data;
	struct block *b = list_entry(conf->block_list.prev,
			struct block, block_entry);
	const char *val = (const char *) event->data.scalar.value;
	char *bad;

	if (YAML_SCALAR_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	b->multiple = (unsigned int) strtoul(val, &bad, 10);
	if (bad[0] != 0)
		fatal("bad integer (%s) in %s at line %zu column %zu\n",
				val,
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);
	debug(" %s\n", val);

	pcs_parser_remove_node(node);
	return 1;
}

static int
block_name_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct server_config *conf = node->state->data;
	struct block *b = list_entry(conf->block_list.prev,
			struct block, block_entry);
	const char *name = (const char *) event->data.scalar.value;

	if (YAML_SCALAR_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	strncpy(b->name, name, PCS_MAX_NAME_LENGTH);
	b->name[PCS_MAX_NAME_LENGTH - 1] = 0;
	debug(" %s\n", name);

	pcs_parser_remove_node(node);
	return 1;
}

static int
end_block_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	const char *key = (const char *) &node[1];
	struct server_config *conf = node->state->data;
	struct block *b = list_entry(conf->block_list.prev,
			struct block, block_entry);

	debug3("%s:%i\n", __FUNCTION__, __LINE__);
	b->ops = b->builder->ops(b);
	if (!b->ops || !b->ops->run)
		fatal("bad config for %s in %s at line %zu column %zu\n",
				key,
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);
	register_output(conf, b);

	return pcs_parser_up(node, event);
}

static int
new_block_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	const char *key = (const char *) &node[1];
	struct server_config *conf = node->state->data;
	struct block_builder *(*loader)(void) = NULL;
	struct block_builder *builder = NULL;
	struct block *b;

	if (YAML_MAPPING_START_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	loader = pcs_lookup(loaders, key);
	if (!loader)
		return pcs_parser_unexpected_key(node, event, (const char *) &node[1]);

	builder = loader();
	if (!builder)
		return pcs_parser_unexpected_key(node, event, (const char *) &node[1]);

	b = xzalloc(sizeof(*b));
	b->multiple = conf->multiple;
	b->counter = 1;
	b->builder = builder;
	b->outputs_table = builder->outputs;
	if (builder->alloc)
		b->data = builder->alloc();

	list_add_tail(&b->block_entry, &conf->block_list);

	node->handler[YAML_SCALAR_EVENT] = pcs_parser_scalar_key;
	node->handler[YAML_MAPPING_START_EVENT] = NULL;
	node->handler[YAML_MAPPING_END_EVENT] = end_block_event;
	node->handler[YAML_SEQUENCE_START_EVENT] = NULL;
	return 1;
}

static int
blocks_start_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct server_config *conf = node->state->data;

	if (YAML_SEQUENCE_START_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	node->handler[YAML_SCALAR_EVENT] = NULL;
	node->handler[YAML_SEQUENCE_START_EVENT] = NULL;
	node->handler[YAML_MAPPING_START_EVENT] = map_sequence_event;
	node->handler[YAML_SEQUENCE_END_EVENT] = pcs_parser_up;
	if (!conf->regs_count)
		conf->regs_count = PCS_DEFAULT_REGS_COUNT;
	conf->regs = xzalloc(sizeof(*conf->regs) *
			conf->regs_count);
	return 1;
}

static struct pcs_parser_map options_map[] = {
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

static struct pcs_parser_map inputs_map[] = {
	{
		.handler		= block_input_event,
	}
	,{
	}
};

static struct pcs_parser_map setpoints_map[] = {
	{
		.handler		= new_setpoint_event,
	}
	,{
	}
};

static struct pcs_parser_map strings_map[] = {
	{
		.handler		= new_string_event,
	}
	,{
	}
};

static struct pcs_parser_map block_map[] = {
	{
		.key			= "input",
		.handler		= block_input_event,
	}
	,{
		.key			= "inputs",
		.handler		= pcs_parser_map,
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
		.handler		= pcs_parser_map,
		.data			= &setpoints_map,
	}
	,{
		.key			= "strings",
		.handler		= pcs_parser_map,
		.data			= &strings_map,
	}
	,{
	}
};

static struct pcs_parser_map blocks_map[] = {
	{
		.handler		= new_block_event,
		.data			= &block_map,
	}
	,{
	}
};

static struct pcs_parser_map top_map[] = {
	{
		.key			= "options",
		.handler		= pcs_parser_map,
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

static struct pcs_parser_map document_map[] = {
	{
		.handler		= pcs_parser_map,
		.data			= &top_map,
	}
	,{
	}
};

static struct pcs_parser_map stream_map = {
		.handler		= pcs_parser_one_document,
		.data			= &document_map,
};

int
load_server_config(const char const *filename, struct server_config *conf)
{
	default_config(conf);

	return pcs_parse_yaml(filename, &stream_map, conf);
}
