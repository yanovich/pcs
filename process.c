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
#include "log.h"
#include "list.h"
#include "process.h"
#include "servconf.h"

int received_sigterm = 0;

static void
sigterm_handler(int sig)
{
	  received_sigterm = sig;
}

int
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

int
b016(int apm)
{
	if (apm < 3937 || apm > 19685)
		return 0x80000000;
	return (1600 * (apm - 3937)) / (19685 - 3937);
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

static struct site_status status = {{0}};
static struct site_config config = {0};

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
			config.DO_mod[a->mod].state &= ~a->digital.mask;
			config.DO_mod[a->mod].state |=  a->digital.value;
			config.DO_mod[a->mod].write(&config.DO_mod[a->mod]);
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

void
load_site_config(const char *config_file)
{
	debug("loading config\n");
	INIT_LIST_HEAD(&config.process_list);
	load_server_config(config_file, &config);
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
load_site_status()
{
	int err;
	int i, j, t;
	int offset = 0;
	char data[256];
	int raw[8] = {0};

	for (i = 0; config.TR_mod[i].count > 0; i++) {
		err = config.TR_mod[i].read(&config.TR_mod[i], raw);
		if (0 > err) {
			error("%s:%i bad temp data: %s(%i)\n", __FILE__,
				       	__LINE__, data, err);
			return -1;
		}
		t = offset;
		offset += config.TR_mod[i].count;
		for (j = 0; t < offset; j++, t++) {
			status.T[t] = PCS_BAD_DATA;
			if (!config.T[t].convert)
				continue;
			status.T[t] = config.T[t].convert(raw[j]);
			if (PCS_BAD_DATA == status.T[t]) {
				error("%s:%i bad temp data: %s(%i)\n", __FILE__,
					       	__LINE__, data, j);
				error("convert: %p\n", config.T[t].convert);
				return -1;
			}
		}
	}

	offset = 0;
	for (i = 0;config.AI_mod[i].count > 0; i++) {
		err = config.AI_mod[i].read(&config.AI_mod[i], raw);
		if (0 > err) {
			error("%s: bad analog data: %s(%i)\n", __FILE__,
				       	data, err);
			return -1;
		}
		t = offset;
		offset += config.AI_mod[i].count;
		for (j = 0; t < offset; j++, t++) {
			status.AI[t] = PCS_BAD_DATA;
			if (!config.AI[t].convert)
				continue;
			status.AI[t] = config.AI[t].convert(raw[j]);
			if (PCS_BAD_DATA == status.AI[t]) {
				error("%s: bad analog data: %s(%i)\n", __FILE__,
					       	data, j);
				return -1;
			}
		}
	}

	offset = 0;
	for (i = 0; config.DO_mod[i].count > 0; i++) {
		err = config.DO_mod[i].read(&config.DO_mod[i]);
		if (0 != err) {
			error("%s: status data\n", __FILE__);
			return -1;
		}
		debug("DO status 0x%08x\n", config.DO_mod[i].state);
		load_DO(offset, config.DO_mod[i].state);
		offset += config.DO_mod[i].count;
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
		if (config.DO_mod[i].count == 0) {
			fatal("Invalid DO index %i\n", index);
			return;
		}
		offset += config.DO_mod[i].count;
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
	list_for_each_entry(p, &config.process_list, process_entry) {
		if (!p->ops->log)
			continue;
		c += p->ops->log(curr, p->config, buff, sz, c);
	}
	logit("%s\n", buff);
}

void
process_loop(void)
{
	struct timeval start, now;
	struct process *p;

	signal(SIGTERM, sigterm_handler);
	signal(SIGQUIT, sigterm_handler);
	signal(SIGINT, sigterm_handler);

	gettimeofday(&start, NULL);
	while (!received_sigterm) {
		while (load_site_status() != 0 && !received_sigterm)
			sleep (1);
		if (received_sigterm)
			return;

		list_for_each_entry(p, &config.process_list, process_entry) {
			if (! p->ops->run)
				fatal("process without run\n");
			p->ops->run(&status, p->config);
		}
		log_status(&status);
		execute_actions(&status);
		gettimeofday(&now, NULL);
		if (received_sigterm)
			return;
		do_sleep(&start, &now, config.interval);
		start.tv_sec += config.interval / 1000000;
	}
}
