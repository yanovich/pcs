/* cascade.c -- control heating system temperature
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

#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>

#include "includes.h"
#include "log.h"
#include "list.h"
#include "process.h"
#include "valve.h"
#include "cascade.h"

struct cascade_config {
	int			input;
	int			p_in;
	int			p_out;
	int			first_run;
	int			m11_fail;
	int			m11_int;
};

static void
cascade_run(struct site_status *s, void *conf)
{
	struct cascade_config *c = conf;
	int m1 = get_DO(1);
	int m2 = get_DO(2);
	int m4 = get_DO(4);

	debug("running cascade\n");

	if (get_DO(3) == 0)
		set_DO(3, 1, 0);
	if (s->T[c->input] > 160) {
		if (m1)
			set_DO(1, 0, 0);
		if (m2)
			set_DO(2, 0, 0);
		if (m4)
			set_DO(4, 0, 0);
		c->m11_int = 0;
		return;
	}

	if (s->T[c->input] > 140)
		return;

	if (!m4)
		set_DO(4, 1, 0);

	if (!m1 && !m2) {
		if (s->T[c->input] % 2)
			set_DO(1, 1, 0);
		else
			set_DO(2, 1, 0);
		c->m11_int = 0;
		return;
	}

	if (c->m11_fail > 1) {
		if (!m1)
			set_DO(1, 1, 0);
		if (!m2)
			set_DO(2, 1, 0);
		c->m11_int = 0;
		return;
	}

	c->m11_int++;

	if (c->m11_int > 10 && (s->AI[c->p_out] - s->AI[c->p_in]) < 40)
		c->m11_fail++;
	else
		c->m11_fail = 0;

	if (c->m11_int > 7 * 24 * 60 * 6) {
		set_DO(1, !m1, 0);
		set_DO(2, !m2, 0);
		c->m11_int = 0;
	}
	return;
}

static int
cascade_log(struct site_status *s, void *conf, char *buff,
		const int sz, int o)
{
	struct cascade_config *c = conf;
	int m1 = get_DO(1);
	int m2 = get_DO(2);
	int m3 = get_DO(3);
	int m4 = get_DO(4);
	int b = 0;

	if (o == sz)
		return 0;
	if (o) {
		buff[o] = ',';
		b++;
	}
	return snprintf(&buff[o + b], sz - o - b,
		       	"P11 %3i P12 %3i %c%c%c%c",
		       	s->AI[c->p_out], s->AI[c->p_in],
		       	m1 ? 'M' : '_',
		       	m2 ? 'M' : '_',
		       	m3 ? 'M' : '_',
		       	m4 ? 'M' : '_'
			);
}

struct process_ops cascade_ops = {
	.run = cascade_run,
	.log = cascade_log,
};

void
load_cascade(struct list_head *list)
{
	struct process *p = (void *) xmalloc (sizeof(*p));
	struct cascade_config *c = (void *) xmalloc (sizeof(*c));

	c->first_run = 1;
	c->input = 4;
	c->p_in = 0;
	c->p_out = 1;
	c->m11_fail = 0;
	c->m11_int = 0;
	p->config = (void *) c;
	p->ops = &cascade_ops;
	list_add_tail(&p->process_entry, list);
}
