/* process.c -- organize controlled processes
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
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#include "includes.h"
#include "fuzzy.h"
#include "icp.h"
#include "log.h"
#include "list.h"
#include "process.h"
#include "hot_water.h"
#include "heating.h"
#include "cascade.h"

int received_sigterm = 0;

static void
sigterm_handler(int sig)
{
	  received_sigterm = sig;
}

static int
ni1000(int ohm)
{
	const int o[] = {7909, 8308, 8717, 9135 , 9562 , 10000 ,
	       	10448 , 10907 , 11140 , 11376 , 11857 , 12350 , 12854 ,
	       	13371 , 13901 , 14444 , 15000 , 15570 , 16154 , 16752 ,
	       	17365 , 17993 };
	int i = sizeof(o) / (2 * sizeof(*o));
	int l = (sizeof(o) / sizeof (*o)) - 2;
	int f = 0;
	do {
		if (o[i] <= ohm)
			if (o[i + 1] > ohm) {
				return (100 * (ohm - o[i])) /
					(o[i + 1] - o[i]) + i * 100 -  500;
			}
			else if (i == l)
				return 0x80000000;
			else {
				f = i;
				i = i + 1 + (l - i - 1) / 2;
			}
		else
			if (i == f)
				return 0x80000000;
			else {
				l = i;
				i = i - 1 - (i - 1 - f) / 2;
			}
	} while (1);
}

static int
b016(int apm)
{
	if (apm < 3937 || apm > 19685)
		return 0x80000000;
	return (1600 * (apm - 3937)) / (19685 - 3937);
}

static int
parse_amps(const char *data, int size, int *press)
{
	int i = 0, point = 0, val = 0;
	const char *p = data;

	if (*p != '+')
		return 0;

	p++;
	while (1) {
		if (*p >= '0' && *p <= '9') {
			val *= 10;
			val += *p - '0';
		} else if ('.' == *p) {
			if (point)
				return i;
			point = 1;
		} else if ('+' == *p || 0 == *p) {
			if (i >= size)
				return size + 1;
			press[i++] = b016(val);
			if (!*p)
				return i;
			val = 0;
			point = 0;
		}
		p++;
	}
}

static int
parse_ohms(const char *data, int size, int *temp)
{
	int i = 0, point = 0, val = 0;
	const char *p = data;

	if (*p != '+')
		return 0;

	p++;
	while (1) {
		if (*p >= '0' && *p <= '9') {
			val *= 10;
			val += *p - '0';
		} else if ('.' == *p) {
			if (point)
				return i;
			point = 1;
		} else if ('+' == *p || 0 == *p) {
			if (i >= size)
				return size + 1;
			temp[i++] = ni1000(val);
			if (!*p)
				return i;
			val = 0;
			point = 0;
		}
		p++;
	}
}

LIST_HEAD(action_list);

static int
compare_actions(struct action *a1, struct action *a2)
{
	if (a1->type > a2->type)
		return 1;
	if (a1->type < a2->type)
		return -1;
	switch (a1->type) {
	case ANALOG_OUTPUT:
		if (a1->data.analog.index > a2->data.analog.index)
			return 1;
		if (a1->data.analog.index > a2->data.analog.index)
			return -1;
		return 0;
	case DIGITAL_OUTPUT:
		if (a1->data.digital.delay > a2->data.digital.delay)
			return 1;
		if (a1->data.digital.delay < a2->data.digital.delay)
			return -1;
		debug("equal action1 %p %08x %08x (%li)\n",
				a1,
			       	a1->data.digital.value,
				a1->data.digital.mask,
				a1->data.digital.delay);
		debug("equal action2 %p %08x %08x (%li)\n",
				a2,
			       	a2->data.digital.value,
				a2->data.digital.mask,
				a2->data.digital.delay);
		return 0;
	default:
		return 0;
	}
}

void
queue_action(struct action *action)
{
	struct action *a, *n;
	int c;
	unsigned value;

	switch (action->type) {
	case ANALOG_OUTPUT:
		break;
	case DIGITAL_OUTPUT:
		debug("checking action %08x %08x (%li)\n",
			       	action->data.digital.value,
				action->data.digital.mask,
				action->data.digital.delay);
		value = action->data.digital.value;
		value &= ~action->data.digital.mask;
		if (action->data.digital.mask == 0)
		       return;
		if (value != 0) {
			error("invalid action %08x %08x (%li)\n",
					action->data.digital.value,
					action->data.digital.mask,
					action->data.digital.delay);
			return;
		}
	}
	n = (void *) xmalloc(sizeof(*n));
	n = memcpy(n, action, sizeof(*n));
	debug("queueing action %08x %08x (%li)\n",
			action->data.digital.value,
			action->data.digital.mask,
			action->data.digital.delay);
	list_for_each_entry(a, &action_list, action_entry) {
		c = compare_actions(n, a);
		if (c == 1)
			continue;
		if (c == -1) {
			list_add_tail(&n->action_entry, &a->action_entry);
			return;
		}
		switch (n->type) {
		case ANALOG_OUTPUT:
			error("duplicate analog output %u\n",
					n->data.analog.index);
			return;
		case DIGITAL_OUTPUT:
			if ((a->data.digital.mask & n->data.digital.mask)
					!= 0) {
				error("overlapping masks %08x %08x\n",
						a->data.digital.mask,
						n->data.digital.mask);
				return;
			}
			a->data.digital.mask |= n->data.digital.mask;
			a->data.digital.value |= n->data.digital.value;
			return;
		default:
			error("unknown action type %u", n->type);
			return;
		}
	}
	if (&a->action_entry == &action_list)
		list_add_tail(&n->action_entry, &action_list);
}

long
do_sleep(struct timeval *base, struct timeval *now, long offset)
{
	long delay;
	if (offset > 100000000) {
		error("offset %li is to high\n", offset);
		return -1;
	}
	debug("delay: %li, %li.%06li %li.%06li\n", offset,
			now->tv_sec, now->tv_usec, base->tv_sec, base->tv_usec);
	delay = offset - now->tv_usec + base->tv_usec
	       	- 1000000 * (now->tv_sec - base->tv_sec);
	debug("delaying %li of %li usec\n", delay, offset);
	if (0 >= delay || delay >= 100000000)
		return 0;
	usleep(delay);
	return offset;
}

unsigned
execute_actions(struct site_status *s)
{
	struct action *a, *n;
	struct timeval start, now;
	long t = 0;
	list_for_each_entry_safe(a, n, &action_list, action_entry) {
		if (received_sigterm)
			return 0;

		switch (a->type) {
		case ANALOG_OUTPUT:
			continue;
		case DIGITAL_OUTPUT:
			gettimeofday(&now, NULL);
			if (!a->data.digital.delay) {
				debug("base:  %li.%06li\n",
					       	start.tv_sec,
						start.tv_usec);
				memcpy(&start, &now, sizeof(start));
				debug("base:  %li.%06li\n",
					       	start.tv_sec,
						start.tv_usec);
			} else {
				t += do_sleep(&start, &now,
					       	a->data.digital.delay);
			}
			debug("[%li.%06li] executing action %08x %08x (%li)\n",
					now.tv_sec,
					now.tv_usec,
					a->data.digital.value,
					a->data.digital.mask,
					a->data.digital.delay);
			s->do0 &= ~a->data.digital.mask;
			s->do0 |=  a->data.digital.value;
			set_parallel_output_status(2, s->do0);
			list_del(&a->action_entry);
			xfree(a);
			continue;
		default:
			continue;
		}
	}
	debug("done executing\n");
	return t;
}

LIST_HEAD(process_list);

void
load_site_config(void)
{
	debug("loading config\n");
	load_heating(&process_list);
	load_hot_water(&process_list);
	load_cascade(&process_list);
	debug("loaded config\n");
}

int
load_site_status(struct site_status *site_status)
{
	int err;
	char data[256];
	int temp[7] = {0};
	int press[8] = {0};
	unsigned output;

	err = icp_serial_exchange(3, "#00", 256, &data[0]);
	if (0 > err) {
		error("%s: temp data read failure\n", __FILE__);
		return err;
	}
	if ('>' != data[0]) {
		error("%s: bad temp data: %s\n", __FILE__, data);
		return -1;
	}
	err = parse_ohms(&data[1], sizeof(temp), temp);
	if (7 != err) {
		error("%s: bad temp data: %s\n", __FILE__, data);
		return -1;
	}
	site_status->t		= temp[4];
	site_status->t11	= temp[0];
	site_status->t12	= temp[1];
	site_status->t21	= temp[3];

	err = icp_serial_exchange(4, "#00", 256, &data[0]);
	if (0 > err) {
		error("%s: pressure data read failure\n", __FILE__);
		return err;
	}

	err = parse_amps(&data[1], sizeof(press), press);
	if (8 != err) {
		error("%s: bad temp data: %s(%i)\n", __FILE__, data, err);
		return -1;
	}
	site_status->p12 = press[0];
	site_status->p11 = press[1];

	err = get_parallel_output_status(2, &output);
	if (0 != err) {
		error("%s: status data\n", __FILE__);
		return -1;
	}

	site_status->do0 = output;
	site_status->v21 = 0;

	return 0;
}

unsigned int
get_DO(int slot, int index)
{
	return 0;
}

void
set_DO(int slot, int index, int value)
{
}

static unsigned
execute_v21(struct site_status *curr)
{
	struct action action = {{0}};
	long usec;

	action.type = DIGITAL_OUTPUT;

	if (-50 < curr->v21 && curr->v21 < 50)
		return 0;

	action.data.digital.mask |= 0x000000C0;
	if (curr->v21 < 0) {
		usec = curr->v21 * -1000;
		action.data.digital.value = 0x00000080;
	} else {
		usec = curr->v21 * 1000;
		action.data.digital.value = 0x00000040;
	}
	if (usec > 5000000)
		usec = 5000000;

	queue_action(&action);

	action.data.digital.delay = usec;
	action.data.digital.value = 0x00000000;
	queue_action(&action);

	return 0;
}

void
log_status(struct site_status *site_status)
{
	logit("T %3i, T11 %3i, T12 %3i, T21 %3i, P11 %3u, P12 %3u, "
			"V1 %4i V2 %4i\n", site_status->t,
		       	site_status->t11, site_status->t12, site_status->t21,
		       	site_status->p11, site_status->p12, 0,
			site_status->v21);
}

static struct site_status status = {0};

void
process_loop(void)
{
	unsigned t;
	struct site_status *curr = &status;
	struct process *p;

	signal(SIGTERM, sigterm_handler);
	signal(SIGQUIT, sigterm_handler);
	signal(SIGINT, sigterm_handler);

	while (!received_sigterm) {
		while (load_site_status(curr) != 0)
			sleep (1);

		list_for_each_entry(p, &process_list, process_entry) {
			if (! p->ops->run) {
				error("process without run\n");
				continue;
			}
			p->ops->run(curr, p->config);
		}
		log_status(curr);
		t = 10000000;
		t -= execute_v21(curr);;
		t -= execute_actions(curr);
		if (received_sigterm)
			return;
		usleep(t);
	}
}
