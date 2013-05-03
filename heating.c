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
	int			v11_abs;
	int			v11_pos;
};

static int
heating_run(struct site_status *curr, void *conf)
{
	struct heating_config *hwc = conf;
	int vars[2];
	int t11, t12, e12, v11;
	struct action action = {{0}};
	long usec;

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

	v11 = process_fuzzy(&hwc->fuzzy, &vars[0]);

	debug("v11 %5i v11_pos %6i\n", v11, hwc->v11_pos);
	v11 += hwc->v11_pos;
	if (hwc->v11_abs == 1) {
		if (v11 < 0) {
			v11 = 0;
		}
		else if (v11 > 47000) {
			hwc->v11_pos = 46000;
		}
	} else {
		if (v11 < -47000) {
			hwc->v11_pos = 1000;
			v11 = 0;
			hwc->v11_abs = 1;
		}
		else if (v11 > 47000) {
			v11 = 47000;
			hwc->v11_pos = 46000;
			hwc->v11_abs = 1;
		}
	}
	v11 -= hwc->v11_pos;

	action.type = DIGITAL_OUTPUT;

	if (-50 < v11 && v11 < 50)
		return 0;

	hwc->v11_pos += v11;

	action.data.digital.mask |= 0x00000030;
	if (v11 < 0) {
		usec = v11 * -1000;
		action.data.digital.value = 0x00000010;
	} else {
		usec = v11 * 1000;
		action.data.digital.value = 0x00000020;
	}
	if (usec > 5000000)
		usec = 5000000;

	queue_action(&action);

	action.data.digital.delay = usec;
	action.data.digital.value = 0x00000000;
	queue_action(&action);

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
	hwc->v11_abs = 0;
	hwc->v11_pos = 0;
	hwp->config = (void *) hwc;
	hwp->class = &heating_class;
	list_add_tail(&hwp->process_entry, list);
}
