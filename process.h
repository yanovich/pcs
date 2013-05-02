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


typedef enum {
	ANALOG_OUTPUT,
	DIGITAL_OUTPUT
} action_type;

struct action {
	struct list_head		action_entry;
	action_type			type;
	union {
		struct {
			unsigned	value;
			unsigned	mask;
			long		delay;
		}			digital;
		struct {
			unsigned	value;
			unsigned	index;
		}			analog;
	}				data;
};

struct site_status {
	int		t;
	int		t11;
	int		t12;
	int		t21;
	unsigned int	p11;
	unsigned int	p12;
	unsigned int	do0;
	int		v11;
	int		v21;
	int		e11;
	int		e12;
	int		e21;
	int		v11_sum;
	int		v21_sum;
	int		v11_pos;
	int		v21_pos;
	int		m11_int;
	int		m11_fail;
};

struct process_class {
	int	(*run)(struct site_status *, struct site_status *, void *);
	int	(*free)(void*);
}; 

struct process {
	struct process_class	*class;
	struct list_head	process_entry;
	void			*config;
};

void
load_site_config(void);

int
load_site_status(struct site_status *site_status);

void
log_status(struct site_status *site_status);

void
process_loop(void);

#endif /* PCS_PROCESS_H */
