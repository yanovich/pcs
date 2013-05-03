/* pcs.c -- process control service
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
#include "hot_water.h"

struct hot_water_config {
	struct list_head	fuzzy;
	int			first_run;
	int			t21_prev;
	struct valve		*valve;
};

static void
hot_water_run(struct site_status *curr, void *conf)
{
	struct hot_water_config *hwc = conf;
	int vars[2];
	int v21;

	debug("running hot water\n");
	if (hwc->first_run) {
		hwc->first_run = 0;
		hwc->t21_prev = curr->t21;
	}
	vars[0] = curr->t21 - 570;
	vars[1] = curr->t21 - hwc->t21_prev;
	hwc->t21_prev = curr->t21;

	v21 = process_fuzzy(&hwc->fuzzy, &vars[0]);
	if (!hwc->valve || !hwc->valve->ops || !hwc->valve->ops->adjust)
		fatal("bad valve\n");
	
	hwc->valve->ops->adjust(v21, hwc->valve->data);
	return;
}

static int
hot_water_log(struct site_status *curr, void *conf, char *buff,
		const int sz, int c)
{
	struct hot_water_config *hwc = conf;
	int b = 0;

	if (c == sz)
		return 0;
	if (c) {
		buff[c] = ',';
		b++;
	}
	b += snprintf(&buff[c + b], sz - c - b, "T21 %3i ", curr->t21);
	if (hwc->valve && hwc->valve->ops && hwc->valve->ops->log)
		b += hwc->valve->ops->log(hwc->valve->data, &buff[c + b],
			       	sz, c + b);

	return b;
}

struct process_ops hot_water_ops = {
	.run = hot_water_run,
	.log = hot_water_log,
};

void
load_hot_water(struct list_head *list)
{
	struct process *hwp = (void *) xmalloc (sizeof(*hwp));
	struct hot_water_config *hwc = (void *) xmalloc (sizeof(*hwc));
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
	hwc->valve = load_2way_valve(50, 5000, 47000, 0, 8, 0, 7);
	hwp->config = (void *) hwc;
	hwp->ops = &hot_water_ops;
	list_add_tail(&hwp->process_entry, list);
}
