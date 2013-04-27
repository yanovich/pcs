#include <stdio.h>
#include <yaml.h>

int parse(yaml_event_t *event)
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

int main()
{
	yaml_parser_t parser;
	yaml_event_t event;
	FILE *f = fopen("test.conf", "rb");
	int done;

	if (NULL == f)
		return 1;

	yaml_parser_initialize(&parser);

	yaml_parser_set_input_file(&parser, f);

	while (!done) {
		if (!yaml_parser_parse(&parser, &event))
			goto error1;

		done = parse(&event);
	}

	yaml_parser_delete(&parser);
	fclose(f);
	return 0;

error1:
	yaml_parser_delete(&parser);
	fclose(f);
	return 1;
}
