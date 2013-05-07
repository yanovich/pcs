/* process.h -- organize controlled processes
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

#ifndef PCS_PROCESS_H
#define PCS_PROCESS_H

#include "modules.h"

unsigned int
get_DO(int index);

void
set_DO(int index, int value, int delay);

#define PCS_BAD_DATA		0x80000000

struct site_status {
	unsigned int		DO[256];
	int			T[256];
	int			AI[256];
};

struct site_config {
	long			interval;
	struct DO_mod		DO_mod[256];
	struct TR_sensor	T[256];
	struct TR_mod		TR_mod[256];
	struct AI_sensor	AI[256];
	struct AI_mod		AI_mod[256];
};

struct process_ops {
	void	(*run)(struct site_status *, void *);
	int	(*log)(struct site_status *, void *, char *, const int, int);
	int	(*free)(void*);
}; 

struct process {
	struct process_ops	*ops;
	struct list_head	process_entry;
	void			*config;
};

void
load_site_config(const char *config_file);

int
load_site_status();

void
log_status(struct site_status *site_status);

void
process_loop(void);

#endif /* PCS_PROCESS_H */
