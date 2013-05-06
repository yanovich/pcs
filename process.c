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

typedef enum {
	ANALOG_OUTPUT,
	DIGITAL_OUTPUT
} action_type;

struct action {
	action_type			type;
	struct list_head		action_entry;
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

static int
compare_actions(struct action *a1, struct action *a2)
{
	if (a1->type > a2->type)
		return 1;
	if (a1->type < a2->type)
		return -1;
	switch (a1->type) {
	case ANALOG_OUTPUT:
		if (a1->mod > a2->mod)
			return 1;
		if (a1->mod < a2->mod)
			return -1;
		if (a1->analog.index > a2->analog.index)
			return 1;
		if (a1->analog.index > a2->analog.index)
			return -1;
		return 0;
	case DIGITAL_OUTPUT:
		if (a1->digital.delay > a2->digital.delay)
			return 1;
		if (a1->digital.delay < a2->digital.delay)
			return -1;
		if (a1->mod > a2->mod)
			return 1;
		if (a1->mod < a2->mod)
			return -1;
		debug("equal action1 %p %08x %08x (%li)\n",
				a1,
			       	a1->digital.value,
				a1->digital.mask,
				a1->digital.delay);
		debug("equal action2 %p %08x %08x (%li)\n",
				a2,
			       	a2->digital.value,
				a2->digital.mask,
				a2->digital.delay);
		return 0;
	default:
		return 0;
	}
}

static void
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
			       	action->digital.value,
				action->digital.mask,
				action->digital.delay);
		value = action->digital.value;
		value &= ~action->digital.mask;
		if (action->digital.mask == 0)
		       return;
		if (value != 0) {
			error("invalid action %08x %08x (%li)\n",
					action->digital.value,
					action->digital.mask,
					action->digital.delay);
			return;
		}
	}
	n = (void *) xmalloc(sizeof(*n));
	n = memcpy(n, action, sizeof(*n));
	debug("queueing action %08x %08x (%li)\n",
			action->digital.value,
			action->digital.mask,
			action->digital.delay);
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
					n->analog.index);
			return;
		case DIGITAL_OUTPUT:
			if ((a->digital.mask & n->digital.mask)
					!= 0) {
				error("overlapping masks %08x %08x\n",
						a->digital.mask,
						n->digital.mask);
				return;
			}
			a->digital.mask |= n->digital.mask;
			a->digital.value |= n->digital.value;
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
	gettimeofday(now, NULL);
	return offset;
}

static struct site_status status = {0};

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
			if (!a->digital.delay) {
				memcpy(&start, &now, sizeof(start));
			} else {
				t += do_sleep(&start, &now,
					       	a->digital.delay);
			}
			debug("[%li.%06li] executing action %08x %08x (%li)\n",
					now.tv_sec,
					now.tv_usec,
					a->digital.value,
					a->digital.mask,
					a->digital.delay);
			status.DO_mod[a->mod].state &= ~a->digital.mask;
			status.DO_mod[a->mod].state |=  a->digital.value;
			status.DO_mod[a->mod].write(&status.DO_mod[a->mod]);
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
	int type, more;
	status.DO_mod[0] = *(struct DO_mod *)
	       	icp_init_module("8041", 0, &type, &more);
	status.DO_mod[0].slot = 2;
	load_heating(&process_list);
	load_hot_water(&process_list);
	load_cascade(&process_list);
	status.interval = 10000000;
	debug("loaded config\n");
}

void
load_DO(int offset, unsigned int value)
{
	int shift = offset % 32;
	int index = offset / 32;

	if (shift == 0) {
		status.DO[index] = value;
		return;
	}
	status.DO[index] &= (1 << shift) - 1;
	status.DO[index] |= value << shift;
	status.DO[index + 1] = value >> shift;
}

int
load_site_status(struct site_status *site_status)
{
	int err;
	int i;
	int offset = 0;
	char data[256];
	int temp[7] = {0};
	int press[8] = {0};

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
	site_status->T[4].t	= temp[4];
	site_status->T[0].t	= temp[0];
	site_status->T[1].t	= temp[1];
	site_status->T[3].t	= temp[3];

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

	for (i = 0; status.DO_mod[i].count > 0; i++) {
		err = status.DO_mod[i].read(&status.DO_mod[i]);
		if (0 != err) {
			error("%s: status data\n", __FILE__);
			return -1;
		}
		load_DO(offset, status.DO_mod[i].state);
		offset += status.DO_mod[i].count;
	}

	return 0;
}

unsigned int
get_DO(int index)
{
	index--;
	return (status.DO[index/8] & 1 << (index % 8)) != 0;
}

void
set_DO(int index, int value, int delay)
{
	struct action action = {0};
	int i = 0, offset = 0;

	index--;
	if (index < 0) {
		fatal("Invalid DO index %i\n", index);
		return;
	}
	action.type = DIGITAL_OUTPUT;
	while (1) {
		if (status.DO_mod[i].count == 0) {
			fatal("Invalid DO index %i\n", index);
			return;
		}
		offset += status.DO_mod[i].count;
		if (offset > index)
			break;
		i++;
	}
	action.mod = i;
	action.digital.delay = delay;
	action.digital.mask |= 1 << (index);
	if (value)
		action.digital.value |= 1 << (index);
	queue_action(&action);
}

void
log_status(struct site_status *curr)
{
	const int sz = 128;
	int c = 0;
	char buff[sz];
	struct process *p;
	buff[0] = 0;
	list_for_each_entry(p, &process_list, process_entry) {
		if (!p->ops->log)
			continue;
		c += p->ops->log(curr, p->config, buff, sz, c);
	}
	logit("%s\n", buff);
}

void
process_loop(void)
{
	struct site_status *curr = &status;
	struct timeval start, now;
	struct process *p;

	signal(SIGTERM, sigterm_handler);
	signal(SIGQUIT, sigterm_handler);
	signal(SIGINT, sigterm_handler);

	gettimeofday(&start, NULL);
	while (!received_sigterm) {
		while (load_site_status(curr) != 0)
			sleep (1);

		list_for_each_entry(p, &process_list, process_entry) {
			if (! p->ops->run)
				fatal("process without run\n");
			p->ops->run(curr, p->config);
		}
		log_status(curr);
		execute_actions(curr);
		gettimeofday(&now, NULL);
		if (received_sigterm)
			return;
		do_sleep(&start, &now, curr->interval);
		start.tv_sec += curr->interval / 1000000;
	}
}
