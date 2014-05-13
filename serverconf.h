/* serverconf.h -- process server configuration definitions
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

#ifndef _PCS_SERVER_CONF_H
#define _PCS_SERVER_CONF_H

#include <sys/time.h>

#include "list.h"
#include "state.h"

#define PCS_DEFAULT_REGS_COUNT	512

struct server_config {
	long			multiple;
	struct list_head	block_list;
	int			regs_count;
	int			regs_used;
	long			*regs;
	struct server_state	state;
};

void
load_server_config(const char const *filename, struct server_config *conf);
#endif
