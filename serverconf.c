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
#include "list.h"
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
};

static void
default_config(struct server_config *conf)
{
	conf->tick.tv_sec = 10;
	conf->tick.tv_usec = 0;
}

static struct pcs_parser_node*
new_parser_node(struct pcs_parser_state *state, struct list_head *parent)
{
	struct pcs_parser_node *n = xzalloc(sizeof(*n));
	if (!state || !parent)
		fatal("cannot create parser node\n");

	n->state = state;
	list_add(&n->node_entry, parent);

	return n;
}

static int
debug_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	debug("%s\n", yaml_event_type[event->type]);
	return 1;
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

static int
end_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	list_del(&node->node_entry);
	xfree(node);
	return 1;
}

static int
exit_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	list_del(&node->node_entry);
	xfree(node);
	return 0;
}

static int
document_start_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	node->handler[YAML_DOCUMENT_START_EVENT] = NULL;
	node->handler[YAML_DOCUMENT_END_EVENT] = end_event;
	node->handler[YAML_SCALAR_EVENT] = debug_event;
	node->handler[YAML_SEQUENCE_START_EVENT] = debug_event;
	node->handler[YAML_SEQUENCE_END_EVENT] = debug_event;
	node->handler[YAML_MAPPING_START_EVENT] = debug_event;
	node->handler[YAML_MAPPING_END_EVENT] = debug_event;
	return 1;
}

static int
stream_start_event(struct pcs_parser_node *node, yaml_event_t *event)
{
	struct pcs_parser_node *n = new_parser_node(node->state,
			&node->node_entry);
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
	struct pcs_parser_node *node = new_parser_node(&state, &nodes);

	if (NULL == f)
		fatal("failed to open %s (%s)\n", filename, strerror(errno));

	node->handler[YAML_STREAM_START_EVENT] = stream_start_event;
	yaml_parser_initialize(&parser);
	yaml_parser_set_input_file(&parser, f);

	while (run) {
		if (!yaml_parser_parse(&parser, &event))
			fatal("failed to parse %s\n", filename);
		debug3("%s\n", yaml_event_type[event.type]);
		node = list_entry(nodes.prev, struct pcs_parser_node,
				node_entry);
		if (&node->node_entry == &nodes)
			fatal("empty parser list\n");
		handler = node->handler[event.type];
		if (!handler)
			handler = unexpected_event;
		run = handler(node, &event);
	}

	fclose(f);
}

void
load_server_config(const char const *filename, struct server_config *conf)
{
	default_config(conf);

	parse_config(filename, conf);
}
