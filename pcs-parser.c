/* pcs-parser.c -- YAML parsing utilities
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

#include "pcs-parser.h"

static const char *yaml_event_type[] = {
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

const char *
pcs_parser_event_type(int i)
{
	if (i < 0 || i > YAML_MAPPING_END_EVENT)
		return "UNKNOWN_EVENT";

	return yaml_event_type[i];
}

struct pcs_parser_node*
pcs_parser_new_node(struct pcs_parser_state *state, struct list_head *parent,
		size_t extra)
{
	struct pcs_parser_node *n = xzalloc(sizeof(*n) + extra);
	if (!state || !parent) {
		error("cannot create parser node\n");
		return NULL;
	}

	n->state = state;
	list_add(&n->node_entry, parent);
	if (extra)
		n->data = &n[1];

	return n;
}

void
pcs_parser_remove_node(struct pcs_parser_node *node)
{
	list_del(&node->node_entry);
	xfree(node);
}

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

int
pcs_parser_unexpected_key(struct pcs_parser_node *node, yaml_event_t *event,
		const char *key)
{
	error("unexpected key %s in %s at line %zu column %zu\n",
			key,
			node->state->filename,
			event->start_mark.line,
			event->start_mark.column);
	return 0;
}

int
pcs_parser_unexpected_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	error("unexpected %s in %s at line %zu column %zu\n",
			pcs_parser_event_type(event->type),
			node->state->filename,
			event->start_mark.line,
			event->start_mark.column);
	return 0;
}

int
pcs_parser_scalar_key(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct pcs_parser_node *n;
	struct pcs_parser_map *map = node->data;
	int (*handler)(struct pcs_parser_node *node, yaml_event_t *event);
	const char *key = (const char*) event->data.scalar.value;
	size_t len = event->data.scalar.length;
	int i;
	char *s;

	if (YAML_SCALAR_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	if (len > 500) {
		error("key (%s) is over 500 chars\n", key);
		return 0;
	}

	len += 1;
	len += sizeof(int *) - len % sizeof(int *);
	n = pcs_parser_new_node(node->state, &node->node_entry, len);
	s = (char *) &n[1];
	strncpy(s, key, len);

	i = handler_from_map(key, map);
	handler = map[i].handler;

	if (!handler)
		return pcs_parser_unexpected_key(node, event, key);

	n->handler[YAML_SCALAR_EVENT] = handler;
	n->handler[YAML_SEQUENCE_START_EVENT] = handler;
	n->handler[YAML_MAPPING_START_EVENT] = handler;
	n->data = map[i].data;
	debug("%s:\n", key);
	return 1;
}

static int
pcs_parser_finish(struct pcs_parser_node *node, yaml_event_t *event)
{
	pcs_parser_remove_node(node);
	return 0;
}

static int
parse_stream(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct pcs_parser_map *map = node->data;
	struct pcs_parser_node *n = pcs_parser_new_node(node->state,
			&node->node_entry, 0);
	if (!n) {
		error("%s: empty node\n", __FUNCTION__);
		return 0;
	}

	if (!map) {
		error("%s: empty map\n", __FUNCTION__);
		return 0;
	}

	node->handler[YAML_STREAM_START_EVENT] = NULL;
	node->handler[YAML_STREAM_END_EVENT] = pcs_parser_finish;
	n->handler[YAML_DOCUMENT_START_EVENT] = map->handler;
	n->data = map->data;
	return 1;
}

int
pcs_parse_yaml(const char *filename, struct pcs_parser_map *map, void *data)
{
	int run = 1;
	FILE *f = fopen(filename, "r");
	yaml_parser_t parser;
	yaml_event_t event;
	int (*handler)(struct pcs_parser_node *node, yaml_event_t *event);
	struct pcs_parser_state state = {
		.filename = filename,
		.data = data,
	};
	LIST_HEAD(nodes);
	struct pcs_parser_node *node = pcs_parser_new_node(&state, &nodes, 0);
	struct pcs_parser_node *tmp;

	if (!node) {
		error("%s: empty node\n", __FUNCTION__);
		return 1;
	}

	if (NULL == f) {
		error("failed to open %s (%s)\n", filename, strerror(errno));
		return 1;
	}

	node->handler[YAML_STREAM_START_EVENT] = parse_stream;
	node->data = map;
	yaml_parser_initialize(&parser);
	yaml_parser_set_input_file(&parser, f);

	while (run) {
		if (!yaml_parser_parse(&parser, &event)) {
			error("failed to parse %s\n", filename);
			break;
		}
		node = list_entry(nodes.prev, struct pcs_parser_node,
				node_entry);
		if (YAML_SCALAR_EVENT != event.type)
			debug3("%s (%p)\n",
					pcs_parser_event_type(event.type),
					node);
		else
			debug3("%s (%p) %s\n",
					pcs_parser_event_type(event.type),
					node,
					event.data.scalar.value);
		if (&node->node_entry == &nodes) {
			error("empty parser list\n");
			return 1;
		}
		handler = node->handler[event.type];
		if (!handler)
			handler = pcs_parser_unexpected_event;
		run = handler(node, &event);
		yaml_event_delete(&event);
	}

	yaml_parser_delete(&parser);
	fclose(f);

	if (nodes.prev == nodes.next)
		return 0;
	list_for_each_entry_safe(node, tmp, &nodes, node_entry) {
		pcs_parser_remove_node(node);
	}
	return 1;
}

int
pcs_parser_up(struct pcs_parser_node *node, yaml_event_t *event)
{
	pcs_parser_remove_node(node);
	return 1;
}

int
pcs_parser_one_document(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct pcs_parser_map *map = node->data;
	struct pcs_parser_node *n = pcs_parser_new_node(node->state,
			&node->node_entry, 0);

	if (!n) {
		error("%s: empty node\n", __FUNCTION__);
		return 0;
	}

	if (!map) {
		error("%s: empty map\n", __FUNCTION__);
		return 0;
	}

	node->handler[YAML_DOCUMENT_START_EVENT] = NULL;
	node->handler[YAML_DOCUMENT_END_EVENT] = pcs_parser_up;
	n->handler[YAML_MAPPING_START_EVENT] = map->handler;
	n->data = map->data;
	return 1;
}

int
pcs_parser_map(struct pcs_parser_node *node, yaml_event_t *event)
{
	if (YAML_MAPPING_START_EVENT != event->type)
		return pcs_parser_unexpected_event(node, event);

	node->handler[YAML_SCALAR_EVENT] = pcs_parser_scalar_key;
	node->handler[YAML_MAPPING_START_EVENT] = NULL;
	node->handler[YAML_MAPPING_END_EVENT] = pcs_parser_up;
	return 1;
}

int
pcs_parser_long(struct pcs_parser_node *node, yaml_event_t *event, int *err)
{
	char *bad;
	const char * from = (const char *) event->data.scalar.value;
	long val = strtol(from, &bad, 0);
	if (bad[0] != 0) {
		error("bad integer(%s) in %s at line %zu column %zu\n",
				from,
				node->state->filename,
				event->start_mark.line,
				event->start_mark.column);
		if (NULL == err)
			fatal("exiting");
		else
			*err = 1;
	}
	return val;
}

