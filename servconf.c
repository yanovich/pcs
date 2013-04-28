#include <stdio.h>
#include <errno.h>
#include <yaml.h>

#include <log.h>

#include "includes.h"

static int
parse(yaml_event_t *event)
{
	int done = (event->type == YAML_STREAM_END_EVENT);
	switch (event->type) {
	case YAML_DOCUMENT_START_EVENT:
		printf("---\n");
		break;
	case YAML_DOCUMENT_END_EVENT:
		printf("...\n");
		break;
	case YAML_SCALAR_EVENT:
		printf("%s\n", event->data.scalar.value);
		break;
	case YAML_SEQUENCE_START_EVENT:
		printf("[\n");
		break;
	case YAML_SEQUENCE_END_EVENT:
		printf("]\n");
		break;
	case YAML_MAPPING_START_EVENT:
		printf("{\n");
		break;
	case YAML_MAPPING_END_EVENT:
		printf("}\n");
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
	int done;

	if (NULL == f)
		fatal("failed to open %s (%s)\n", filename, strerror(errno));

	yaml_parser_initialize(&parser);

	yaml_parser_set_input_file(&parser, f);

	while (!done) {
		if (!yaml_parser_parse(&parser, &event))
			fatal("failed to parse %s\n", filename);

		done = parse(&event);
	}

	yaml_parser_delete(&parser);
	fclose(f);
}

int
main()
{
	log_init(PACKAGE_NAME, SYSLOG_LEVEL_INFO, SYSLOG_FACILITY_DAEMON, 1);
	load_server_config("test.conf", NULL);
	return 0;
}
