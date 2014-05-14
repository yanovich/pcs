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

	if (len > 500)
		fatal("key (%s) is over 500 chars\n", key);

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

