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
#include "valve.h"

unsigned int
get_DO(int index);

unsigned int
get_DI(int index);

void
set_DO(int index, int value, int delay);

void
set_AO(int index, int value);

#define PCS_BAD_DATA		0x80000000

struct site_status {
	unsigned int		DO[256];
	int			DI[256];
	unsigned int		DS[256];
	int			AI[256];
};

typedef enum {
	INPUT,
	LOG,
	PROCESS,
	ANALOG_OUTPUT,
	DIGITAL_OUTPUT
} process_type;

struct process {
	struct list_head		process_entry;
	struct process_ops		*ops;
	void				*config;
	process_type			type;
	struct timeval			start;
	size_t				interval;
	int				mod;
	union {
		struct {
			long		delay;
			unsigned	value;
			unsigned	mask;
		}			digital;
		struct {
			unsigned	value;
			unsigned	index;
		}			analog;
	};
};

struct process_ops {
	void	(* free)(struct process *);
	void	(* run)(struct process *, struct site_status *);
	int	(* update)(struct process *, struct process *);
	int	(* log)(struct site_status *, void *, char *, const int, int);
}; 

struct process_builder {
	void			*(*alloc)(void);
	struct setpoint_map	*setpoint;
	struct io_map		*io;
	struct list_head *	(*fuzzy)(void *config);
	void			(*set_valve)(void *config, struct valve *v);
	void *			(*ops)(void *config);
};

int
ni1000(int ohm);

int
pt1000(int ohm);

int
b013(int apm);

int
b016(int apm);

int
b031(int apm);

int
v2h(int volts);

int
v2h2(int volts);

void
load_site_config(const char *config_file);

int
load_site_status();

void
log_status(struct site_status *site_status);

void
queue_process(struct process *n);

void
process_loop(void);

#endif /* PCS_PROCESS_H */
