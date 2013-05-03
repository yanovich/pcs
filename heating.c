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
#include "heating.h"

struct heating_config {
	struct list_head	fuzzy;
	int			first_run;
	int			e11_prev;
	int			e12_prev;
};

static int
heating_run(struct site_status *curr, void *conf)
{
	struct heating_config *hwc = conf;
	int vars[2];
	int t11, t12, e12;

	if (curr->t > 160) {
		t12 = curr->t12;
		t11 = 300;
	} else {
		t12 = 700 - ((curr->t + 280) * 250) / 400;
		t11 = 950 - ((curr->t + 280) * 450) / 400;
	}
	e12 = curr->t12 - t12;
	if (e12 > 0)
		t11 -= e12;

	vars[0] = curr->t11 - t11;
	if (hwc->first_run) {
		hwc->first_run = 0;
		hwc->e11_prev = vars[0];
		hwc->e12_prev = e12 > 0 ? e12 : 0;
	}
	vars[1] = vars[0] - hwc->e11_prev;
	if (e12 > 0)
		vars[1] -= e12 - hwc->e12_prev;
	hwc->e11_prev = vars[0];
	hwc->e12_prev = e12 > 0 ? e12 : 0;

	debug("running heating\n");

	curr->v11 = process_fuzzy(&hwc->fuzzy, &vars[0]);
	return 0;
}

struct process_class heating_class = {
	.run = heating_run,
};

void
load_heating(struct list_head *list)
{
	struct process *hwp = (void *) xmalloc (sizeof(*hwp));
	struct heating_config *hwc = (void *) xmalloc (sizeof(*hwc));
	struct fuzzy_clause *fcl;

	INIT_LIST_HEAD(&hwc->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 2, -1500, -50,  -30, 0,   400,  7000, 7000);
	list_add_tail(&fcl->fuzzy_entry, &hwc->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 0,   -50, -30,  -10, 0,   100,   400,  700);
	list_add_tail(&fcl->fuzzy_entry, &hwc->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 0,   -30, -10,    0, 0,     0,   100,  200);
	list_add_tail(&fcl->fuzzy_entry, &hwc->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 0,   -10,   0,   30, 0,  -300,     0,  300);
	list_add_tail(&fcl->fuzzy_entry, &hwc->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 0,     0,  30,   80, 0,  -200,  -100,    0);
	list_add_tail(&fcl->fuzzy_entry, &hwc->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 0,    30,  80,  180, 0,  -700,  -400, -100);
	list_add_tail(&fcl->fuzzy_entry, &hwc->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 0, 1,    80, 180, 1500, 0, -7000, -7000, -400);
	list_add_tail(&fcl->fuzzy_entry, &hwc->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 1, 2, -1000, -10,   -1, 0,   100,   400,  700);
	list_add_tail(&fcl->fuzzy_entry, &hwc->fuzzy);
	fcl = (void *) xmalloc(sizeof(*fcl));
	fuzzy_clause_init(fcl, 1, 1,     1,  10, 1000, 0,  -700,  -400, -100);
	list_add_tail(&fcl->fuzzy_entry, &hwc->fuzzy);

	hwc->first_run = 1;
	hwp->config = (void *) hwc;
	hwp->class = &heating_class;
	list_add_tail(&hwp->process_entry, list);
}
