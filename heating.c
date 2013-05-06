/* heating.c -- control heating system temperature
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
#include "fuzzy.h"
#include "log.h"
#include "list.h"
#include "process.h"
#include "valve.h"
#include "heating.h"

struct heating_config {
	int			street;
	int			feed;
	int			flowback;
	struct list_head	fuzzy;
	int			first_run;
	int			e11_prev;
	int			e12_prev;
	struct valve		*valve;
};

static void
heating_run(struct site_status *s, void *conf)
{
	struct heating_config *c = conf;
	int vars[2];
	int t11, t12, e12, v11;

	if (s->T[c->street].t > 160) {
		t12 = s->T[c->flowback].t;
		t11 = 300;
	} else {
		t12 = 700 - ((s->T[c->street].t + 280) * 250) / 400;
		t11 = 950 - ((s->T[c->street].t + 280) * 450) / 400;
	}
	e12 = s->T[c->flowback].t - t12;
	if (e12 > 0)
		t11 -= e12;

	vars[0] = s->T[c->feed].t - t11;
	if (c->first_run) {
		c->first_run = 0;
		c->e11_prev = vars[0];
		c->e12_prev = e12 > 0 ? e12 : 0;
	}
	vars[1] = vars[0] - c->e11_prev;
	if (e12 > 0)
		vars[1] -= e12 - c->e12_prev;
	c->e11_prev = vars[0];
	c->e12_prev = e12 > 0 ? e12 : 0;

	debug("running heating\n");

	v11 = process_fuzzy(&c->fuzzy, &vars[0]);

	if (!c->valve || !c->valve->ops || !c->valve->ops->adjust)
		fatal("bad valve\n");
	
	c->valve->ops->adjust(v11, c->valve->data);
	return;
}

static int
heating_log(struct site_status *s, void *conf, char *buff,
		const int sz, int o)
{
	struct heating_config *c = conf;
	int b = 0;

	if (o == sz)
		return 0;
	if (o) {
		buff[o] = ',';
		b++;
	}
	b += snprintf(&buff[o + b], sz - o - b, "T %3i T11 %3i T12 %3i ",
			s->T[c->street].t, s->T[c->feed].t, s->T[c->flowback].t);
	if (c->valve && c->valve->ops && c->valve->ops->log)
		b += c->valve->ops->log(c->valve->data, &buff[o + b],
			       	sz, o + b);

	return b;
}

struct process_ops heating_ops = {
	.run = heating_run,
	.log = heating_log,
};

void
load_heating(struct list_head *list)
{
	struct process *hwp = (void *) xmalloc (sizeof(*hwp));
	struct heating_config *c = (void *) xmalloc (sizeof(*c));
	struct fuzzy_clause *fcl;

	INIT_LIST_HEAD(&c->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 2, -1500, -50,  -30, 0,   400,  7000, 7000);
	list_add_tail(&fcl->fuzzy_entry, &c->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 0,   -50, -30,  -10, 0,   100,   400,  700);
	list_add_tail(&fcl->fuzzy_entry, &c->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 0,   -30, -10,    0, 0,     0,   100,  200);
	list_add_tail(&fcl->fuzzy_entry, &c->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 0,   -10,   0,   30, 0,  -300,     0,  300);
	list_add_tail(&fcl->fuzzy_entry, &c->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 0,     0,  30,   80, 0,  -200,  -100,    0);
	list_add_tail(&fcl->fuzzy_entry, &c->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 0,    30,  80,  180, 0,  -700,  -400, -100);
	list_add_tail(&fcl->fuzzy_entry, &c->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 1,    80, 180, 1500, 0, -7000, -7000, -400);
	list_add_tail(&fcl->fuzzy_entry, &c->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 1, 2, -1000, -10,   -1, 0,   100,   400,  700);
	list_add_tail(&fcl->fuzzy_entry, &c->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 1, 1,     1,  10, 1000, 0,  -700,  -400, -100);
	list_add_tail(&fcl->fuzzy_entry, &c->fuzzy);

	c->street = 4;
	c->feed = 0;
	c->flowback = 1;
	c->first_run = 1;
	c->valve = load_2way_valve(50, 5000, 47000, 5, 6);
	hwp->config = (void *) c;
	hwp->ops = &heating_ops;
	list_add_tail(&hwp->process_entry, list);
}
