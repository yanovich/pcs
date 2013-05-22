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

LIST_HEAD(process_list);

static struct site_status status = {{0}};
static struct site_config config = {0};

static void
process_analog_out_run(struct process *a, struct site_status *s)
{
	config.AO_mod[a->mod].write(&config.AO_mod[a->mod],
			a->analog.index, a->analog.value);
}

static int
process_analog_out_update(struct process *n, struct process *a)
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

static struct process_ops analog_out_ops = {
	.run 			= process_analog_out_run,
	.update 		= process_analog_out_update,
};

static void
process_digital_out_run(struct process *a, struct site_status *s)
{
	config.DO_mod[a->mod].state &= ~a->digital.mask;
	config.DO_mod[a->mod].state |=  a->digital.value;
	config.DO_mod[a->mod].write(&config.DO_mod[a->mod]);
}

static int
process_digital_out_update(struct process *n, struct process *a)
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

static struct process_ops digital_out_ops = {
	.run	 		= process_digital_out_run,
	.update 		= process_digital_out_update,
};

static int
compare_processes(struct process *n, struct process *a)
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
process_free(struct process *a)
{
	if (a->ops->free)
		a->ops->free(a);
	else
		xfree(a);
}

static void
queue_process(struct process *n)
{
	struct process *a;
	int c;

	list_for_each_entry(a, &process_list, process_entry) {
		c = compare_processes(n, a);
		if (c == 0) {
			process_free(n);
			return;
		}
		if (c == 1)
			continue;
		if (c == -1) {
			list_add_tail(&n->process_entry, &a->process_entry);
			return;
		}
	}
	if (&a->process_entry == &process_list)
		list_add_tail(&n->process_entry, &process_list);
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
	struct process *process = (void *) xzalloc(sizeof(*process));
	int i = 0, offset = 0;

	index--;
	if (index < 0) {
		fatal("Invalid DO index %i\n", index);
		return;
	}
	process->type = DIGITAL_OUTPUT;
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
	process->mod = i;
	process->digital.delay = delay;
	process->digital.mask |= 1 << (index);
	if (value)
		process->digital.value |= 1 << (index);
	gettimeofday(&process->start, NULL);
	process->start.tv_usec += delay;
	process->start.tv_sec  += process->start.tv_usec / 1000000;
	process->start.tv_usec %= 1000000;
	process->ops = &digital_out_ops;
	queue_process(process);
}

void
set_AO(int index, int value)
{
	struct process *process = (void *) xzalloc(sizeof(*process));
	int i = 0, offset = 0;

	if (index < 0) {
		fatal("Invalid AO index %i\n", index);
		return;
	}
	process->type = ANALOG_OUTPUT;
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
	process->mod = i;
	process->analog.index = index;
	process->analog.index -= config.AO_mod[i].first;
	process->analog.value = value;
	gettimeofday(&process->start, NULL);
	process->ops = &analog_out_ops;
	queue_process(process);
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

static void
input_loop(struct process *a, struct site_status *s)
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
input_update(struct process *n, struct process *a)
{
	return 1;
}

struct process_ops process_ops = {
	.run			= input_loop,
	.update			= input_update,
};

void process_wait(struct process *e)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	do_sleep(&e->start, &now);
}

int process_requeue(struct process *e)
{
	if (e->interval == 0)
		return 0;

	e->start.tv_sec += e->interval / 1000000;
	queue_process(e);
	return 1;
}

void
process_loop(void)
{
	struct process *e;

	signal(SIGTERM, sigterm_handler);
	signal(SIGQUIT, sigterm_handler);
	signal(SIGINT, sigterm_handler);

	e = (void *) xzalloc(sizeof(*e));
	e->ops = &process_ops;
	e->interval = config.interval;
	e->type = PROCESS;
	gettimeofday(&e->start, NULL);

	queue_process(e);

	while (!received_sigterm) {
		e = container_of(process_list.next, typeof(*e), process_entry);
		if (&e->process_entry == &process_list)
			fatal("no active process\n");
		process_wait(e);
		if (received_sigterm)
			return;
		e->ops->run(e, &status);
		list_del(&e->process_entry);
		if (!process_requeue(e)) {
			if (e->ops->free)
				e->ops->free(e);
			else
				xfree(e);
		}
	}
}
