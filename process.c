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

int
v2h(int volts)
{
	return (volts * 8) / 10;
}

LIST_HEAD(action_list);

struct action {
	process_type			type;
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
	struct action_ops	* ops;
	struct timeval		start;
	size_t			interval;
};

struct action_ops {
	void			(* free)(struct action *);
	void			(* run)(struct action *, struct site_status *);
	int			(* update)(struct action *, struct action *);
};

static struct site_status status = {{0}};
static struct site_config config = {0};

static void
action_analog_out_run(struct action *a, struct site_status *s)
{
	config.AO_mod[a->mod].write(&config.AO_mod[a->mod],
			a->analog.index, a->analog.value);
}

static int
action_analog_out_update(struct action *n, struct action *a)
{
	if (n->mod > a->mod)
		return 1;
	if (n->mod < a->mod)
		return -1;
	if (n->analog.index > a->analog.index)
		return 1;
	if (n->analog.index < a->analog.index)
		return -1;
	return 0;
}

static struct action_ops analog_out_ops = {
	.run 			= action_analog_out_run,
	.update 		= action_analog_out_update,
};

static void
action_digital_out_run(struct action *a, struct site_status *s)
{
	config.DO_mod[a->mod].state &= ~a->digital.mask;
	config.DO_mod[a->mod].state |=  a->digital.value;
	config.DO_mod[a->mod].write(&config.DO_mod[a->mod]);
}

static int
action_digital_out_update(struct action *n, struct action *a)
{
	if (n->mod > a->mod)
		return 1;
	if (n->mod < a->mod)
		return -1;
	if ((a->digital.mask & n->digital.mask)
			!= 0) {
		error("overlapping masks %08x %08x\n",
				a->digital.mask,
				n->digital.mask);
		return 0;
	}
	a->digital.mask |= n->digital.mask;
	a->digital.value |= n->digital.value;
	return 0;
}

static struct action_ops digital_out_ops = {
	.run	 		= action_digital_out_run,
	.update 		= action_digital_out_update,
};

static int
compare_actions(struct action *n, struct action *a)
{
	long delay = n->start.tv_usec - a->start.tv_usec + 
	       	1000000 * (n->start.tv_sec - a->start.tv_sec);
	if (delay > 1000)
		return 1;
	if (delay < -1000)
		return -1;
	if (n->type > a->type)
		return 1;
	if (n->type < a->type)
		return -1;
	return a->ops->update(n, a);
}

static void
action_free(struct action *a)
{
	if (a->ops->free)
		a->ops->free(a);
	else
		xfree(a);
}

static void
queue_action(struct action *n)
{
	struct action *a;
	int c;

	list_for_each_entry(a, &action_list, action_entry) {
		c = compare_actions(n, a);
		if (c == 0) {
			action_free(n);
			return;
		}
		if (c == 1)
			continue;
		if (c == -1) {
			list_add_tail(&n->action_entry, &a->action_entry);
			return;
		}
	}
	if (&a->action_entry == &action_list)
		list_add_tail(&n->action_entry, &action_list);
}

long
do_sleep(struct timeval *base, struct timeval *now)
{
	long delay = base->tv_usec - now->tv_usec + 
	       	1000000 * (base->tv_sec - now->tv_sec);
	if (0 >= delay)
		return 0;
	usleep(delay);
	gettimeofday(now, NULL);
	return delay;
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
	return (status.DO[index/32] & 1 << (index % 32)) != 0;
}

void
set_DO(int index, int value, int delay)
{
	struct action *action = (void *) xzalloc(sizeof(*action));
	int i = 0, offset = 0;

	index--;
	if (index < 0) {
		fatal("Invalid DO index %i\n", index);
		return;
	}
	action->type = DIGITAL_OUTPUT;
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
	action->mod = i;
	action->digital.delay = delay;
	action->digital.mask |= 1 << (index);
	if (value)
		action->digital.value |= 1 << (index);
	gettimeofday(&action->start, NULL);
	action->start.tv_usec += delay;
	action->start.tv_sec  += action->start.tv_usec / 1000000;
	action->start.tv_usec %= 1000000;
	action->ops = &digital_out_ops;
	queue_action(action);
}

void
set_AO(int index, int value)
{
	struct action *action = (void *) xzalloc(sizeof(*action));
	int i = 0, offset = 0;

	if (index < 0) {
		fatal("Invalid AO index %i\n", index);
		return;
	}
	action->type = ANALOG_OUTPUT;
	while (1) {
		if (config.AO_mod[i].count == 0) {
			fatal("Invalid DO index %i\n", index);
			return;
		}
		offset += config.AO_mod[i].count;
		if (offset > index)
			break;
		i++;
	}
	action->mod = i;
	action->analog.index = index;
	action->analog.index -= config.AO_mod[i].first;
	action->analog.value = value;
	gettimeofday(&action->start, NULL);
	action->ops = &analog_out_ops;
	queue_action(action);
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
process_loop(struct action *a, struct site_status *s)
{
	struct process *p;

	while (load_site_status() != 0 && !received_sigterm)
		sleep (1);
	if (received_sigterm)
		return;

	list_for_each_entry(p, &config.process_list, process_entry) {
		if (! p->ops->run)
			fatal("process without run\n");
		p->ops->run(p, &status);
	}
	log_status(&status);
}

static int
process_update(struct action *n, struct action *a)
{
	return 1;
}

struct action_ops action_ops = {
	.run			= process_loop,
	.update			= process_update,
};

void action_wait(struct action *e)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	do_sleep(&e->start, &now);
}

int action_requeue(struct action *e)
{
	if (e->interval == 0)
		return 0;

	e->start.tv_sec += e->interval / 1000000;
	queue_action(e);
	return 1;
}

void
action_loop(void)
{
	struct action *e;

	signal(SIGTERM, sigterm_handler);
	signal(SIGQUIT, sigterm_handler);
	signal(SIGINT, sigterm_handler);

	e = (void *) xzalloc(sizeof(*e));
	e->ops = &action_ops;
	e->interval = config.interval;
	e->type = PROCESS;
	gettimeofday(&e->start, NULL);

	queue_action(e);

	while (!received_sigterm) {
		e = container_of(action_list.next, typeof(*e), action_entry);
		if (&e->action_entry == &action_list)
			fatal("no active action\n");
		action_wait(e);
		if (received_sigterm)
			return;
		e->ops->run(e, &status);
		list_del(&e->action_entry);
		if (!action_requeue(e)) {
			if (e->ops->free)
				e->ops->free(e);
			else
				xfree(e);
		}
	}
}
