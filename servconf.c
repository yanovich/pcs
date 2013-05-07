/* servconf.c -- load configuration from a YAML file
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

#include <stdio.h>
#include <errno.h>
#include <yaml.h>

#include "log.h"

#include "includes.h"
#include "list.h"
#include "process.h"
#include "icp.h"
#include "hot_water.h"
#include "heating.h"
#include "cascade.h"
#include "servconf.h"

#if 0
static int
parse(yaml_event_t *event, struct config_node *top)
{
	int done = (event->type == YAML_STREAM_END_EVENT);
	struct config_node *node;
	int i;

	debug("%p %i %i %i\n", top, top->level, top->subnode, top->type);
	debug2("%p %p %p\n", top->node_entry.prev, &top->node_entry, top->node_entry.next);
	switch (event->type) {
	case YAML_NO_EVENT:
		break;
	case YAML_STREAM_START_EVENT:
		break;
	case YAML_STREAM_END_EVENT:
		break;
	case YAML_DOCUMENT_START_EVENT:
		if (top->level != 0)
			fatal("Unexpected '---'\n");
		printf("---");
		break;
	case YAML_DOCUMENT_END_EVENT:
		if (top->level != 0)
			fatal("Unexpected '...'\n");
		printf("\n...\n");
		break;
	case YAML_ALIAS_EVENT:
		break;
	case YAML_SCALAR_EVENT:
		if (top->subnode && (top->type == YAML_SEQUENCE_START_EVENT
			       	|| (top->type == YAML_MAPPING_START_EVENT
					&& top->subnode % 2 == 0)))
			printf(",");
		printf("\n");
		for (i = 0; i < top->level; i++)
			printf("  ");
		if (top->type == YAML_MAPPING_START_EVENT) {
			if (top->subnode % 2)
				printf(": ");
			else
				printf("? ");
		}
		printf("\"%s\"", event->data.scalar.value);
		top->subnode++;
		break;
	case YAML_SEQUENCE_START_EVENT:
		node = (struct config_node *) xmalloc(sizeof(*node));
		list_add_tail(&node->node_entry, &top->node_entry);
		node->level = top->level + 1;
		if (top->subnode && (top->type == YAML_SEQUENCE_START_EVENT
			       	|| (top->type == YAML_MAPPING_START_EVENT
					&& top->subnode % 2 == 0)))
			printf(",");
		printf("\n");
		for (i = 0; i < top->level; i++)
			printf("  ");
		if (top->type == YAML_MAPPING_START_EVENT) {
			if ((top->subnode % 2) == 1)
				printf(": ");
			else
				printf("? ");
		}
		node->type = YAML_SEQUENCE_START_EVENT;
		node->subnode = 0;
		printf("[");
		top->subnode++;
		break;
	case YAML_SEQUENCE_END_EVENT:
		printf("\n");
		node = top;
		top = list_entry(top->node_entry.next, struct config_node,
				node_entry);
		list_del(&node->node_entry);
		xfree(node);
		for (i = 0; i < top->level; i++)
			printf("  ");
		printf("]");
		break;
	case YAML_MAPPING_START_EVENT:
		node = (struct config_node *) xmalloc(sizeof(*node));
		list_add_tail(&node->node_entry, &top->node_entry);
		node->level = top->level + 1;
		if (top->subnode && (top->type == YAML_SEQUENCE_START_EVENT
			       	|| (top->type == YAML_MAPPING_START_EVENT
					&& top->subnode % 2 == 0)))
			printf(",");
		printf("\n");
		for (i = 0; i < top->level; i++)
			printf("  ");
		if (top->type == YAML_MAPPING_START_EVENT) {
			if (top->subnode % 2)
				printf(": ");
			else
				printf("? ");
		}
		node->type = YAML_MAPPING_START_EVENT;
		node->subnode = 0;
		printf("{");
		top->subnode++;
		break;
	case YAML_MAPPING_END_EVENT:
		node = top;
		top = list_entry(top->node_entry.next, struct config_node,
				node_entry);
		list_del(&node->node_entry);
		xfree(node);
		printf("\n");
		for (i = 0; i < top->level; i++)
			printf("  ");
		printf("}");
		break;
	};
	yaml_event_delete(event);
	return done;
}
#endif

int
parse_do_nothing(yaml_event_t *event, struct site_config *conf,
	       struct config_node *node)
{
	int done = 0;

	switch (event->type) {
	case YAML_NO_EVENT:
	case YAML_STREAM_START_EVENT:
	case YAML_DOCUMENT_START_EVENT:
	case YAML_DOCUMENT_END_EVENT:
	case YAML_ALIAS_EVENT:
	case YAML_SCALAR_EVENT:
		break;
	case YAML_STREAM_END_EVENT:
		done = 1;
		break;
	case YAML_SEQUENCE_START_EVENT:
	case YAML_MAPPING_START_EVENT:
		node->data++;
		break;
	case YAML_SEQUENCE_END_EVENT:
	case YAML_MAPPING_END_EVENT:
		node->data--;
		break;
	};

	yaml_event_delete(event);
	if (!node->data)
		list_del(&node->node_entry);
	return done;
}

struct config_node empty = {
	.parse = parse_do_nothing,
};

struct top_level_parser {
	int		in_a_map;
};

int
parse_top_level(yaml_event_t *event, struct site_config *conf,
	       struct config_node *node)
{
	int done = 0;
	struct top_level_parser *data = node->data;
	switch (event->type) {
	case YAML_NO_EVENT:
	case YAML_STREAM_START_EVENT:
	case YAML_DOCUMENT_START_EVENT:
	case YAML_DOCUMENT_END_EVENT:
		break;
	case YAML_STREAM_END_EVENT:
		done = 1;
		break;
	case YAML_ALIAS_EVENT:
	case YAML_SEQUENCE_START_EVENT:
	case YAML_SEQUENCE_END_EVENT:
		fatal("unexpected element at line %i column %i\n",
			       	event->start_mark.line,
				event->start_mark.column);
		break;
	case YAML_SCALAR_EVENT:
		debug("configuring %s\n", event->data.scalar.value);
		empty.data = 0;
		list_add(&empty.node_entry, &node->node_entry);
		break;
	case YAML_MAPPING_START_EVENT:
		if (data->in_a_map)
			fatal("unexpected element at line %i column %i\n",
					event->start_mark.line,
					event->start_mark.column);
		data->in_a_map = 1;
		break;
	case YAML_MAPPING_END_EVENT:
		data->in_a_map = 0;
		break;
	};

	yaml_event_delete(event);
	return done;
}


void
load_server_config(const char *filename, struct site_config *conf)
{
	yaml_parser_t parser;
	yaml_event_t event;
	FILE *f = fopen(filename, "r");
	int type, more;
	struct top_level_parser data = {
		.in_a_map	= 0,
	};
	struct config_node top_level = {
		.parse		= parse_top_level,
		.data		= &data,
	};
	struct config_node *node;

	INIT_LIST_HEAD(&top_level.node_entry);

	if (NULL == f)
		fatal("failed to open %s (%s)\n", filename, strerror(errno));

	yaml_parser_initialize(&parser);

	yaml_parser_set_input_file(&parser, f);

	while (1) {
		if (!yaml_parser_parse(&parser, &event))
			fatal("failed to parse %s\n", filename);
		node = list_entry(top_level.node_entry.prev,
			       	struct config_node, node_entry);
		if (!node->parse)
			fatal("%s:%i internal error\n", __FILE__, __LINE__);

		if (node->parse(&event, conf, node))
			break;
	}

	yaml_parser_delete(&parser);
	fclose(f);
	conf->DO_mod[0] = *(struct DO_mod *)
	       	icp_init_module("8041", 0, &type, &more);
	conf->DO_mod[0].slot = 2;
	conf->TR_mod[0] = *(struct TR_mod *)
	       	icp_init_module("87015", 0, &type, &more);
	conf->TR_mod[0].slot = 3;
	conf->T[0].convert	= ni1000;
	conf->T[1].convert	= ni1000;
	conf->T[3].convert	= ni1000;
	conf->T[4].convert	= ni1000;
	conf->AI_mod[0] = *(struct AI_mod *)
	       	icp_init_module("87017", 0, &type, &more);
	conf->AI_mod[0].slot = 4;
	conf->AI[0].convert	= b016;
	conf->AI[1].convert	= b016;
	load_heating(&conf->process_list);
	load_hot_water(&conf->process_list);
	load_cascade(&conf->process_list);
	conf->interval = 10000000;
}
