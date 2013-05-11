/* servconf.h -- load configuration from a YAML file
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

#ifndef PCS_SERVCONF_H
#define PCS_SERVCONF_H

#include <yaml.h>
#include "list.h"

struct site_config {
	long			interval;
	struct DO_mod		DO_mod[256];
	struct TR_sensor	T[256];
	struct TR_mod		TR_mod[256];
	struct AI_sensor	AI[256];
	struct AI_mod		AI_mod[256];
	struct list_head	process_list;
};

struct config_node {
	int			(*parse)(yaml_event_t *event,
		       			struct site_config *conf,
					struct config_node *node);
	struct list_head 	node_entry;
};

struct setpoint_map {
	const char		*name;
	void			(*set)(void *config, int value);
};

struct io_map {
	const char		*name;
	void			(*set)(void *config, int type, int value);
};

void
load_server_config(const char *filename, struct site_config *conf);

#endif /* PCS_SERVCONF_H */
