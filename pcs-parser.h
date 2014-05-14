/* pcs-parser.h -- YAML parsing utilities
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

#ifndef _PCS_PARSER_H
#define _PCS_PARSER_H

#include <yaml.h>

#include "list.h"

const char *
pcs_parser_event_type(int i);

struct pcs_parser_state {
	const char const	*filename;
	void			*data;
};

struct pcs_parser_node {
	struct list_head		node_entry;
	struct pcs_parser_state		*state;
	int				(*handler[YAML_MAPPING_END_EVENT + 1])(
			struct pcs_parser_node *node, yaml_event_t *event);
	void				*data;
	int				sequence;
};

struct pcs_parser_map {
	const char		*key;
	int			(*handler)(
			struct pcs_parser_node *node, yaml_event_t *event);
	void			*data;
};

int
pcs_parse_yaml(const char *filename, struct pcs_parser_map *map, void *data);

struct pcs_parser_node*
pcs_parser_new_node(struct pcs_parser_state *state, struct list_head *parent,
		size_t extra);

void
pcs_parser_remove_node(struct pcs_parser_node *node);

int
pcs_parser_up(struct pcs_parser_node *node, yaml_event_t *event);

int
pcs_parser_one_document(struct pcs_parser_node *node, yaml_event_t *event);

int
pcs_parser_map(struct pcs_parser_node *node, yaml_event_t *event);

int
pcs_parser_scalar_key(struct pcs_parser_node *node, yaml_event_t *event);

int
pcs_parser_unexpected_key(struct pcs_parser_node *node, yaml_event_t *event,
		const char *key);

int
pcs_parser_unexpected_event(struct pcs_parser_node *node, yaml_event_t *event);

#endif
