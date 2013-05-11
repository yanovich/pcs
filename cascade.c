/* cascade.c -- control cascade system temperature
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

#include "includes.h"
#include "log.h"
#include "list.h"
#include "process.h"
#include "cascade.h"

#define HAS_BLOCK_SP		0x01
#define HAS_UNBLOCK_SP		0x02
#define HAS_BLOCK_IO		0x04
#define HAS_BLOCK		0x07

#define HAS_BEFORE_IO		0x01
#define HAS_AFTER_IO		0x02
#define HAS_STAGE_SP		0x04
#define HAS_UNSTAGE_SP		0x08
#define HAS_TARGET		0x0E

struct cascade_config {
	int			block;
	int			block_sp;
	int			unblock_sp;
	int			stage;
	int			unstage;
	int			entry;
	int			feed;
	int			first_run;
	int			motor[256];
	int			motor_count;
	unsigned		has_analog_block;
	unsigned		has_target;
	int			m11_fail;
	int			m11_int;
};

static void
cascade_run(struct site_status *s, void *conf)
{
	struct cascade_config *c = conf;
	struct timeval tv;
	int go = 1;
	int i;
	int j = 0;
	int on = 0;
	int val;
#if 0
	int m1 = get_DO(1);
	int m2 = get_DO(2);
	int m4 = get_DO(4);

	debug("running cascade\n");

	if (get_DO(3) == 0)
		set_DO(3, 1, 0);
	if (s->T[c->block] > 160) {
		if (m1)
			set_DO(1, 0, 0);
		if (m2)
			set_DO(2, 0, 0);
		if (m4)
			set_DO(4, 0, 0);
		c->m11_int = 0;
		return;
	}

	if (s->T[c->block] > 140)
		return;

	if (!m4)
		set_DO(4, 1, 0);

	if (!m1 && !m2) {
		if (s->T[c->block] % 2)
			set_DO(1, 1, 0);
		else
			set_DO(2, 1, 0);
		c->m11_int = 0;
		return;
	}

	if (c->m11_fail > 1) {
		if (!m1)
			set_DO(1, 1, 0);
		if (!m2)
			set_DO(2, 1, 0);
		c->m11_int = 0;
		return;
	}

	c->m11_int++;

	if (c->m11_int > 10 && (s->AI[c->p_out] - s->AI[c->p_in]) < 40)
		c->m11_fail++;
	else
		c->m11_fail = 0;

	if (c->m11_int > 7 * 24 * 60 * 6) {
		set_DO(1, !m1, 0);
		set_DO(2, !m2, 0);
		c->m11_int = 0;
	}
#endif
	if ((c->has_analog_block & HAS_BLOCK) == HAS_BLOCK) {
		if (c->block_sp > c->unblock_sp) {
		       if (c->unblock_sp > s->T[c->block])
			       go = 1;
		       else if (s->T[c->block] > c->block_sp)
			       go = -1;
		       else
			       go = 0;
		} else if (c->block_sp < c->unblock_sp) {
		       if (c->unblock_sp < s->T[c->block])
			       go = 1;
		       else if (s->T[c->block] < c->block_sp)
			       go = -1;
		       else
			       go = 0;
		}
	}
	if (go == 0)
		return;

	if (go == -1) {
		for (i = 0; i < c->motor_count; i++)
			if (get_DO(c->motor[i]))
				set_DO(c->motor[i], 0, 0);
		return;
	}

	if ((c->has_target & HAS_TARGET) != HAS_TARGET) {
		for (i = 0; i < c->motor_count; i++)
			if (!get_DO(c->motor[i]))
				set_DO(c->motor[i], 1, 0);
		return;
	}

	val = s->AI[c->feed];
	if (c->has_target & HAS_BEFORE_IO)
		val -= s->AI[c->entry];

	if (c->unstage > c->stage) {
		if (c->stage > val)
			go = 1;
		else if (val > c->unstage)
			go = -1;
		else
			go = 0;
	} else if (c->unstage < c->stage) {
		if (c->stage < val)
			go = 1;
		else if (val < c->unstage)
			go = -1;
		else
			go = 0;
	}

	if (go == 0)
		return;

	for (i = 0; i < c->motor_count; i++)
		if (get_DO(c->motor[i])) {
			on++;
			j = i;
		}

	if (go == -1) {
		if (on == 1)
			return;
		for (i = 0; i < c->motor_count; i++)
			if (get_DO(c->motor[i])) {
				set_DO(c->motor[i], 0, 0);
				return;
			}
	}

	if (on == c->motor_count)
		return;
	if (on == 0) {
		gettimeofday(&tv, NULL);
		j = tv.tv_sec;
	}
	i = (j + 1) % c->motor_count;
	set_DO(c->motor[i], 1, 0);
	return;
}

static int
cascade_log(struct site_status *s, void *conf, char *buff,
		const int sz, int o)
{
	struct cascade_config *c = conf;
	int b = o;
	int i;

	if (o == sz)
		return 0;
	if (o) {
		buff[o] = ',';
		b++;
	}
	for (i = 0; (i < c->motor_count) && (b < sz); i++, b++) {
		buff[b] = get_DO(c->motor[i]) ? 'M' : '_';
	}
	buff[b] = 0;
	if ((c->has_target & HAS_TARGET) == HAS_TARGET)
		b += snprintf(&buff[b], sz - b, " I %3i O %3i",
				s->AI[c->entry], s->AI[c->feed]);
	return b - o;
}

struct process_ops cascade_ops = {
	.run = cascade_run,
	.log = cascade_log,
};

void
load_cascade(struct list_head *list)
{
	struct process *p = (void *) xmalloc (sizeof(*p));
	struct cascade_config *c = (void *) xmalloc (sizeof(*c));

	c->first_run = 1;
	c->block = 4;
	c->m11_fail = 0;
	c->m11_int = 0;
	p->config = (void *) c;
	p->ops = &cascade_ops;
	list_add_tail(&p->process_entry, list);
}

struct process_ops *
cascade_init(void *conf)
{
	struct cascade_config *c = conf;

	c->first_run = 1;
	return &cascade_ops;
}

static void
set_stage(void *conf, int value)
{
	struct cascade_config *c = conf;
	c->stage = value;
	c->has_target |= HAS_STAGE_SP;
	debug("  cascade: stage = %i\n", value);
}

static void
set_unstage(void *conf, int value)
{
	struct cascade_config *c = conf;
	c->unstage = value;
	c->has_target |= HAS_UNSTAGE_SP;
	debug("  cascade: unstage = %i\n", value);
}

static void
set_block(void *conf, int value)
{
	struct cascade_config *c = conf;
	c->block_sp = value;
	c->has_analog_block |= HAS_BLOCK_SP;
	debug("  cascade: block_sp = %i\n", value);
}

static void
set_unblock(void *conf, int value)
{
	struct cascade_config *c = conf;
	c->unblock_sp = value;
	c->has_analog_block |= HAS_UNBLOCK_SP;
	debug("  cascade: unblock_sp = %i\n", value);
}

struct setpoint_map cascade_setpoints[] = {
	{
		.name 		= "stage",
		.set		= set_stage,
	},
	{
		.name 		= "unstage",
		.set		= set_unstage,
	},
	{
		.name 		= "block",
		.set		= set_block,
	},
	{
		.name 		= "unblock",
		.set		= set_unblock,
	},
	{
	}
};

static void
set_block_io(void *conf, int type, int value)
{
	struct cascade_config *c = conf;
	if (type != TR_MODULE)
		fatal("cascade: wrong type of feed sensor\n");
	c->block = value;
	c->has_analog_block |= HAS_BLOCK_IO;
	debug("  cascade: block io = %i\n", value);
}

static void
set_p_in_io(void *conf, int type, int value)
{
	struct cascade_config *c = conf;
	if (type != AI_MODULE)
		fatal("cascade: wrong type of entry sensor\n");
	c->entry = value;
	c->has_target |= HAS_BEFORE_IO;
	debug("  cascade: entry io = %i\n", value);
}

static void
set_p_out_io(void *conf, int type, int value)
{
	struct cascade_config *c = conf;
	if (type != AI_MODULE)
		fatal("cascade: wrong type of feed sensor\n");
	c->feed = value;
	c->has_target |= HAS_AFTER_IO;
	debug("  cascade: feed io = %i\n", value);
}

static void
add_motor(void *conf, int type, int value)
{
	struct cascade_config *c = conf;
	if (type != DO_MODULE)
		fatal("cascade: wrong type of motor output\n");
	c->motor[c->motor_count++] = value;
	debug("  cascade: motor[%i] = %i\n", c->motor_count - 1, value);
}

struct io_map cascade_io[] = {
	{
		.name 		= "block",
		.set		= set_block_io,
	},
	{
		.name 		= "entry",
		.set		= set_p_in_io,
	},
	{
		.name 		= "feed",
		.set		= set_p_out_io,
	},
	{
		.name 		= "motor",
		.set		= add_motor,
	},
	{
	}
};

static void *
c_alloc(void)
{
	struct cascade_config *c = xzalloc(sizeof(*c));
	return c;
}

struct process_builder cascade_builder = {
	.alloc			= c_alloc,
	.setpoint		= cascade_setpoints,
	.io			= cascade_io,
	.ops			= cascade_init,
};

struct process_builder *
load_cascade_builder(void)
{
	return &cascade_builder;
}
