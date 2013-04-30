#include <stdio.h>
#include <errno.h>
#include <yaml.h>

#include <log.h>

#include "includes.h"
#include "list.h"

struct server_config {
};

struct config_node {
	struct list_head node_entry;
	yaml_event_type_t type;
	int level;
	int subnode;
};

static int
parse(yaml_event_t *event, struct config_node *top)
{
	int done = (event->type == YAML_STREAM_END_EVENT);
	struct config_node *node;
	int i;

	debug("%x %i %i %i\n", top, top->level, top->subnode, top->type);
	debug2("%x %x %x\n", top->node_entry.prev, &top->node_entry, top->node_entry.next);
	switch (event->type) {
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

void
load_server_config(const char *filename, struct server_config *conf)
{
	yaml_parser_t parser;
	yaml_event_t event;
	FILE *f = fopen(filename, "r");
	struct config_node bottom;
	int done;

	INIT_LIST_HEAD(&bottom.node_entry);
	bottom.level = 0;
	bottom.type = YAML_DOCUMENT_START_EVENT;
	bottom.subnode = 0;
	if (NULL == f)
		fatal("failed to open %s (%s)\n", filename, strerror(errno));

	yaml_parser_initialize(&parser);

	yaml_parser_set_input_file(&parser, f);

	while (!done) {
		if (!yaml_parser_parse(&parser, &event))
			fatal("failed to parse %s\n", filename);

		done = parse(&event, list_entry(bottom.node_entry.next,
				       	struct config_node, node_entry));
	}

	yaml_parser_delete(&parser);
	fclose(f);
}

int
main()
{
	log_init(PACKAGE_NAME, LOG_NOTICE, LOG_DAEMON, 1);
	load_server_config("test.conf", NULL);
	return 0;
}
