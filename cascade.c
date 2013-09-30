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
	int			stage_wait;
	int			unstage_wait;
	int			stage_int;
	int			unstage_int;
	int			entry;
	int			entry_type;
	int			feed;
	int			feed_type;
	int			power;
	int			manual;
	int			remote_manual;
	int			status;
	int			first_run;
	int			motor[256];
	int			motor_count;
	unsigned		has_analog_block;
	unsigned		has_target;
	int			m11_fail;
	int			m11_int;
	int			stage_mark;
	int			unstage_mark;
	int			stage_mark_int;
	int			unstage_mark_int;
	int			next_stop;
	int			next_start;
	int			run_min;
	int			run_max;
};

static void
cascade_run(struct process *p, struct site_status *s)
{
	struct cascade_config *c = p->config;
	int go = 1;
	int shutdown = 0;
	char *reason = "shutdown\n";
	int i;
	int j = 0;
	int on = 0;
	int val;
	int last_stage_mark = c->stage_mark;
	int last_unstage_mark = c->unstage_mark;
	
	c->stage_mark = c->stage_wait;
	c->unstage_mark = c->unstage_wait;

	if (c->stage_mark_int)
		c->stage_mark_int--;
	if (c->unstage_mark_int)
		c->unstage_mark_int--;

	if (last_stage_mark)
		last_stage_mark--;
	if (last_unstage_mark)
		last_unstage_mark--;

	debug2("running cascade\n");
	if ((c->power && get_DI(c->power))) {
		shutdown = 1;
		reason = "power failed\n";
		debug2("%s", reason);
	}
	if ((c->manual && get_DI(c->manual))) {
		shutdown = 1;
		reason = "manual override\n";
		debug2("%s", reason);
	}
	if ((c->remote_manual && get_DI(c->remote_manual))) {
		shutdown = 1;
		reason = "remote manual override\n";
		debug2("%s", reason);
	}
	if (!shutdown && (c->has_analog_block & HAS_BLOCK) == HAS_BLOCK) {
		if (c->block_sp > c->unblock_sp) {
		       if (c->unblock_sp > s->AI[c->block])
			       go = 1;
		       else if (s->AI[c->block] > c->block_sp)
			       shutdown = 1;
		       else
			       go = 0;
		} else if (c->block_sp < c->unblock_sp) {
		       if (c->unblock_sp < s->AI[c->block])
			       go = 1;
		       else if (s->AI[c->block] < c->block_sp)
			       shutdown = 1;
		       else
			       go = 0;
		}
	}
	if (shutdown) {
		for (i = 0; i < c->motor_count; i++)
			if (get_DO(c->motor[i])) {
				set_DO(c->motor[i], 0, 0);
				j++;
				c->unstage_mark_int = c->unstage_int;
			}
		if (c->status && get_DO(c->status))
			set_DO(c->status, 0, 0);
		if (j && reason)
			logit("%s", reason);
		return;
	}
	debug2("  cascade: after block go %i\n", go);
	if (go == 0)
		return;

	if ((c->has_target & HAS_TARGET) != HAS_TARGET) {
		for (i = 0; i < c->motor_count; i++)
			if (!get_DO(c->motor[i])) {
				debug2("  motor %i[%i]: %i\n", i,
					       	c->motor[i],
					       	get_DO(c->motor[i]));
				if (c->stage_mark_int) {
					debug("   waiting %i cycles\n",
							c->stage_mark_int);
					return;
				}
				c->stage_mark = last_stage_mark;
				if (c->stage_mark) {
					debug("   waiting %i cycles\n",
							c->stage_mark);
					return;
				}
				set_DO(c->motor[i], 1, 0);
				c->stage_mark_int = c->stage_int;
			}
		return;
	}

	if (c->feed_type == AI_MODULE) {
		val = s->AI[c->feed];
		if (c->has_target & HAS_BEFORE_IO)
			val -= s->AI[c->entry];

	} else if (c->feed_type == DI_STATUS) {
		val = s->DS[c->feed];
	} else {
		error("cascade: bad feed type");
		return;
	}

	debug2("  cascade: target processing %i (%i)\n", val, c->feed_type);
	if (c->unstage > c->stage) {
		if (c->stage >= val)
			go = 1;
		else if (val >= c->unstage)
			go = -1;
		else
			go = 0;
	} else if (c->unstage < c->stage) {
		if (c->stage <= val)
			go = 1;
		else if (val <= c->unstage)
			go = -1;
		else
			go = 0;
	}
	debug2("  cascade: after target go %i\n", go);

	for (i = 0; i < c->motor_count; i++)
		if (get_DO(c->motor[i]))
			on++;

	if (on < c->run_min)
		go = 1;
	else if (on > c->run_max)
		go = -1;
	else if ((on + go) < c->run_min)
		go = 0;
	else if ((on + go) > c->run_max)
		go = 0;

	debug2("  cascade: after limits go %i\n", go);

	if (go == 0)
		return;

	for (i = 0; i < c->motor_count; i++)
		if (get_DO(c->motor[i]))
			on++;

	if (go == -1) {
		for (i = 0; i < c->motor_count; i++) {
			j = (i + c->next_stop) % c->motor_count;
			if (get_DO(c->motor[j])) {
				if (c->unstage_mark_int) {
					debug("   waiting %i cycles\n",
							c->unstage_mark_int); 
					return;
				}
				c->unstage_mark = last_unstage_mark;
				if (c->unstage_mark) {
					debug("   waiting %i cycles\n",
							c->unstage_mark); 
					return;
				}
				if ((on + go) == 0) {
					if (c->status && get_DO(c->status))
						set_DO(c->status, 0, 0);
				}

				set_DO(c->motor[j], 0, 0);
				c->unstage_mark_int = c->unstage_int;
				c->next_stop = j + 1;
			}
		}
	}

	for (i = 0; i < c->motor_count; i++) {
		j = (i + c->next_start) % c->motor_count;
		if (!get_DO(c->motor[j])) {
			if (c->stage_mark_int) {
				debug("   waiting %i cycles\n",
						c->stage_mark_int);
				return;
			}
			c->stage_mark = last_stage_mark;
			if (c->stage_mark) {
				debug("   waiting %i cycles\n",
						c->stage_mark);
				return;
			}
			if ((on + go) > 0) {
				if (c->status && !get_DO(c->status))
					set_DO(c->status, 1, 0);
			}

			set_DO(c->motor[j], 1, 0);
			c->stage_mark_int = c->stage_int;
			c->next_start = j + 1;
		}
	}
}

static int
cascade_log(struct site_status *s, void *conf, char *buff,
		const int sz, int o)
{
	struct cascade_config *c = conf;
	int b = o;
	int i;
	int val;

	if (o == sz)
		return 0;
	if (o) {
		buff[o] = ',';
		b++;
	}
	if (c->status)
		buff[b++] = get_DO(c->status) ? 'S' : '_';
	for (i = 0; (i < c->motor_count) && (b < sz); i++, b++) {
		buff[b] = get_DO(c->motor[i]) ? 'M' : '_';
	}
	buff[b] = 0;
	if ((c->has_target & HAS_TARGET) != HAS_TARGET)
		return b - o;
	if (c->feed_type == AI_MODULE)
		val = s->AI[c->feed];
	else if (c->feed_type == DI_STATUS)
		val = s->DS[c->feed];
	else {
		error("cascade: bad feed type");
		return b - o;
	}
	b += snprintf(&buff[b], sz - b, " O %3i", val);

	if ((c->has_target & HAS_BEFORE_IO) != HAS_BEFORE_IO)
		return b - o;

	if (c->entry_type == AI_MODULE)
		val = s->AI[c->entry];
	else {
		error("cascade: bad entry type");
		return b - o;
	}

	b += snprintf(&buff[b], sz - b, " I %3i", val);

	return b - o;
}

struct process_ops cascade_ops = {
	.run = cascade_run,
	.log = cascade_log,
};

static void *
cascade_init(void *conf)
{
	struct cascade_config *c = conf;

	c->first_run = 1;
	if (c->run_max > c->motor_count)
		c->run_max = c->motor_count;

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
set_stage_wait(void *conf, int value)
{
	struct cascade_config *c = conf;
	c->stage_wait = value;
	debug("  cascade: stage wait = %i\n", value);
}

static void
set_unstage_wait(void *conf, int value)
{
	struct cascade_config *c = conf;
	c->unstage_wait = value;
	debug("  cascade: unstage wait = %i\n", value);
}

static void
set_stage_wait_int(void *conf, int value)
{
	struct cascade_config *c = conf;
	c->stage_int = value;
	debug("  cascade: stage interval = %i\n", value);
}

static void
set_unstage_wait_int(void *conf, int value)
{
	struct cascade_config *c = conf;
	c->unstage_int = value;
	debug("  cascade: unstage interval = %i\n", value);
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

static void
set_run_min(void *conf, int value)
{
	struct cascade_config *c = conf;
	c->run_min = value;
	debug("  cascade: run_min = %i\n", value);
}

static void
set_run_max(void *conf, int value)
{
	struct cascade_config *c = conf;
	c->run_max = value;
	debug("  cascade: run_max = %i\n", value);
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
		.name 		= "stage wait",
		.set		= set_stage_wait,
	},
	{
		.name 		= "unstage wait",
		.set		= set_unstage_wait,
	},
	{
		.name 		= "stage interval",
		.set		= set_stage_wait_int,
	},
	{
		.name 		= "unstage interval",
		.set		= set_unstage_wait_int,
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
		.name 		= "run min",
		.set		= set_run_min,
	},
	{
		.name 		= "run max",
		.set		= set_run_max,
	},
	{
	}
};

static void
set_block_io(void *conf, int type, int value)
{
	struct cascade_config *c = conf;
	if (type != AI_MODULE)
		fatal("cascade: wrong type of feed sensor\n");
	c->block = value;
	c->has_analog_block |= HAS_BLOCK_IO;
	debug("  cascade: block io = %i\n", value);
}

static void
set_power_io(void *conf, int type, int value)
{
	struct cascade_config *c = conf;
	if (type != DI_MODULE)
		fatal("cascade: wrong type of power sensor\n");
	c->power = value;
	debug("  cascade: power io = %i\n", value);
}

static void
set_manual_io(void *conf, int type, int value)
{
	struct cascade_config *c = conf;
	if (type != DI_MODULE)
		fatal("cascade: wrong type of manual sensor\n");
	c->manual = value;
	debug("  cascade: manual io = %i\n", value);
}

static void
set_remote_manual_io(void *conf, int type, int value)
{
	struct cascade_config *c = conf;
	if (type != DI_MODULE)
		fatal("cascade: wrong type of remote manual sensor\n");
	c->remote_manual = value;
	debug("  cascade: remote_manual io = %i\n", value);
}

static void
set_p_in_io(void *conf, int type, int value)
{
	struct cascade_config *c = conf;
	if (type != AI_MODULE)
		fatal("cascade: wrong type of entry sensor\n");
	c->entry = value;
	c->entry_type = type;
	c->has_target |= HAS_BEFORE_IO;
	debug("  cascade: entry io = %i\n", value);
}

static void
set_p_out_io(void *conf, int type, int value)
{
	struct cascade_config *c = conf;
	if (type != AI_MODULE && type != DI_STATUS)
		fatal("cascade: wrong type of feed sensor\n");
	c->feed = value;
	c->feed_type = type;
	c->has_target |= HAS_AFTER_IO;
	debug("  cascade: feed io = %i\n", value);
}

static void
add_status(void *conf, int type, int value)
{
	struct cascade_config *c = conf;
	if (type != DO_MODULE)
		fatal("cascade: wrong type of status output\n");
	c->status = value;
	debug("  cascade: status = %i\n", value);
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
		.name 		= "power",
		.set		= set_power_io,
	},
	{
		.name 		= "manual",
		.set		= set_manual_io,
	},
	{
		.name 		= "remote manual",
		.set		= set_remote_manual_io,
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
		.name 		= "status",
		.set		= add_status,
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
	c->run_max = 256;
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
