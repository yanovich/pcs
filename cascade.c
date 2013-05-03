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
	int			first_run;
	int			m11_fail;
	int			m11_int;
};

static void
cascade_run(struct site_status *curr, void *conf)
{
	struct cascade_config *c = conf;
	struct action *action = (void*) xmalloc(sizeof(*action));

	debug("running cascade\n");
	action->type = DIGITAL_OUTPUT;
	action->data.digital.delay = 0;
	action->data.digital.value = 0x00000000;
	action->data.digital.mask = 0x00000000;

	if ((curr->do0 & 0x00000004) == 0) {
		action->data.digital.value |= 0x00000004;
		action->data.digital.mask |= 0x00000004;
	}
	if (curr->t > 160) {
		if ((curr->do0 & 0x0000000b) != 0)
			action->data.digital.mask |= 0x0000000b;
		queue_action(action);
		c->m11_int = 0;
		return;
	}

	if (curr->t > 140 && (curr->do0 & 0x3) == 0) {
		queue_action(action);
		return;
	}

	c->m11_int++;

	if ((curr->do0 & 0x00000008) == 0) {
		action->data.digital.value |= 0x00000008;
		action->data.digital.mask |= 0x00000008;
	}

	if (c->m11_int > 10 && (curr->p11 - curr->p12) < 40)
		c->m11_fail++;
	else
		c->m11_fail = 0;

	if (c->m11_fail > 1) {
		action->data.digital.value |= 0x00000003;
		action->data.digital.mask |= 0x00000003;
		c->m11_int = 0;
		queue_action(action);
		return;
	}

	if ((curr->do0 & 0x00000003) == 0) {
		action->data.digital.value |= 1 << (curr->t % 2);
		action->data.digital.mask |= 0x00000003;
		c->m11_int = 0;
	} else if (c->m11_int > 7 * 24 * 60 * 6) {
		action->data.digital.value |= (curr->do0 ^ 0x3) & 0x3;
		action->data.digital.mask |= 0x00000003;
		c->m11_int = 0;
	}
	queue_action(action);
	return;
}

struct process_ops cascade_ops = {
	.run = cascade_run,
};

void
load_cascade(struct list_head *list)
{
	struct process *p = (void *) xmalloc (sizeof(*p));
	struct cascade_config *c = (void *) xmalloc (sizeof(*c));

	c->first_run = 1;
	c->m11_fail = 0;
	c->m11_int = 0;
	p->config = (void *) c;
	p->ops = &cascade_ops;
	list_add_tail(&p->process_entry, list);
}
