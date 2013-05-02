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
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#include "config.h"
#include "pathnames.h"
#include "icp.h"
#include "log.h"
#include "list.h"

const char *pid_file = PCS_PID_FILE_PATH;
const char *config_file_name;

int received_sigterm = 0;
int no_detach_flag = 0;

static void
sigterm_handler(int sig)
{
	  received_sigterm = sig;
}

static int
get_parallel_output_status(unsigned int slot, unsigned *status)
{
	int fd, err;
	char buff[256];
	char *p;
	int i = 0;

	if (slot == 0 || slot > 8) {
		error("%s %u: bad slot (%u)\n", __FILE__, __LINE__, slot);
		return -1;
	}
	err = snprintf(&buff[0], sizeof(buff) - 1,
		       	"/sys/bus/icpdas/devices/slot%02u/output_status", slot);
	if (err >= sizeof(buff)) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	fd = open(buff, O_RDWR);
	if (-1 == fd) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}

	err = read(fd, buff, 256);
	if(0 > err) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__,
				strerror(errno), errno);
		goto close_fd;
	}

	err = 0;
	*status = strtoul(buff, &p, 16);
close_fd:
	close(fd);
	return err;
}

static int
set_parallel_output_status(unsigned int slot, unsigned  status)
{
	int fd, err;
	char buff[256];
	char *p;
	int i = 0;

	if (slot == 0 || slot > 8) {
		error("%s %u: bad slot (%u)\n", __FILE__, __LINE__, slot);
		return -1;
	}
	err = snprintf(&buff[0], sizeof(buff) - 1,
		       	"/sys/bus/icpdas/devices/slot%02u/output_status", slot);
	if (err >= sizeof(buff)) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	fd = open(buff, O_RDWR);
	if (-1 == fd) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}

	err = snprintf(&buff[0], sizeof(buff) - 1, "0x%08x", status);
	if (err >= sizeof(buff)) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	err = write(fd, buff, strlen(buff));
	if(0 > err) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__,
				strerror(errno), errno);
		goto close_fd;
	}

	err = 0;
close_fd:
	close(fd);
	return err;
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
	int i = 0, j, point = 0, val = 0;
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
	int i = 0, j, point = 0, val = 0;
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

typedef enum {
	ANALOG_OUTPUT,
	DIGITAL_OUTPUT
} action_type;

struct action {
	struct list_head		action_entry;
	action_type			type;
	union {
		struct {
			unsigned	value;
			unsigned	mask;
			long		delay;
		}			digital;
		struct {
			unsigned	value;
			unsigned	index;
		}			analog;
	}				data;
};

LIST_HEAD(action_list);

int
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
		debug("equal action1 %08x %08x %08x (%u)\n",
				a1,
			       	a1->data.digital.value,
				a1->data.digital.mask,
				a1->data.digital.delay);
		debug("equal action2 %08x %08x %08x (%u)\n",
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
	case DIGITAL_OUTPUT:
		debug("checking action %08x %08x (%u)\n",
			       	action->data.digital.value,
				action->data.digital.mask,
				action->data.digital.delay);
		value = action->data.digital.value;
		value &= ~action->data.digital.mask;
		if (action->data.digital.mask == 0)
		       return;
		if (value != 0) {
			error("invalid action %08x %08x (%u)\n",
					action->data.digital.value,
					action->data.digital.mask,
					action->data.digital.delay);
			return;
		}
	}
	n = (void *) xmalloc(sizeof(*n));
	n = memcpy(n, action, sizeof(*n));
	debug("queueing action %08x %08x (%u)\n",
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

struct site_status {
	int		t;
	int		t11;
	int		t12;
	int		t21;
	unsigned int	p11;
	unsigned int	p12;
	unsigned int	do0;
	int		v11;
	int		v21;
	int		e11;
	int		e12;
	int		e21;
	int		v11_sum;
	int		v21_sum;
	int		v11_pos;
	int		v21_pos;
	int		m11_int;
	int		m11_fail;
};

long
do_sleep(struct timeval *base, struct timeval *now, long offset)
{
	long delay;
	if (offset > 100000000) {
		error("offset %u is to high\n", offset);
		return -1;
	}
	debug("delay: %li, %li.%06li %li.%06li\n", offset,
			now->tv_sec, now->tv_usec, base->tv_sec, base->tv_usec);
	delay = offset - now->tv_usec + base->tv_usec
	       	- 1000000 * (now->tv_sec - base->tv_sec);
	debug("delaying %li of %li usec\n", delay, offset);
	if (0 >= delay && delay >= 100000000)
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
			debug("[%li.%06li] executing action %08x %08x (%u)\n",
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

struct process {
	int		(*run)(struct site_status*, struct site_status*, void*);
	struct list_head	process_entry;
	void		*process_config;
};

LIST_HEAD(process_list);

static int
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
	site_status->v11 = 0;
	site_status->v21 = 0;

	return 0;
};

struct fuzzy_result {
	unsigned	mass;
	int		value;
};

static unsigned
Dh(int a, int b, int c, int x)
{
	if (x < a || x > c || a > b || b > c)
		return 0;
	if (x == b)
		return 0x10000;
	if (a <= x && x < b)
		return (0x10000 * (x - a)) / (b - a);
	else
		return (0x10000 * (c - x)) / (c - b);
}

static void
Dm(int a, int b, int c, unsigned h, struct fuzzy_result *result)
{
	unsigned mass;
	int value;
	int tmp1, tmp2;

	if (a > b || b > c)
		return;
	mass =  (h * (unsigned) (c - a)) / 0x20000;
	value = (c + a) / 2 + ((((int) h) * (2 * b - c - a)) / (int) 0x60000);
	value -= result->value;
	result->mass += mass;
	if (result->mass != 0)
		result->value += (value * (int) mass) / (int) result->mass;
}

static unsigned
Sh(int a, int b, int c, int x)
{
	if (x < a || x > c || a > b || b > c)
		return 0;
	if (x >= b)
		return 0x10000;

	return (0x10000 * (x - a)) / (b - a);
}

static unsigned
Zh(int a, int b, int c, int x)
{
	if (x < a || x > c || a > b || b > c)
		return 0;
	if (x <= b)
		return 0x10000;

	return (0x10000 * (c - x)) / (c - b);
}

static void
calculate_e1(struct site_status *curr)
{
	int t11, t12;

	if (curr->t > 160) {
		t12 = curr->t12;
		t11 = 300;
	} else {
		t12 = 700 - ((curr->t + 280) * 250) / 400;
		t11 = 950 - ((curr->t + 280) * 450) / 400;
	}
	curr->e12 = curr->t12 - t12;
	if (curr->e12 > 0)
		t11 -= curr->e12;

	curr->e11 = curr->t11 - t11;
}

static void
calculate_v11(struct site_status *curr, struct site_status *prev)
{
	int e11 = curr->e11, d11 = e11 - prev->e11;
	struct fuzzy_result res = {0};
	unsigned h;

	if (curr->e12 > 0)
		d11 -= curr->e12 - prev->e12;

	h =  Zh(-1000,-50,  -30, e11);
	Dm(  400, 5000, 5000, h, &res);
	debug2("T11 IS   BN: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh( -50, -30,  -10, e11);
	Dm(  100, 400, 700, h, &res);
	debug2("T11 IS    N: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh( -30, -10,    0, e11);
	Dm(    0, 100,  200, h, &res);
	debug2("T11 IS   SN: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh( -10,   0,   30, e11);
	Dm( -300,   0,  300, h, &res);
	debug2("T11 IS Zero: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh(   0,  30,   80, e11);
	Dm( -200, -100, 0, h, &res);
	debug2("T11 IS   SP: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh(  30,  80,  180, e11);
	Dm( -700, -400, -100, h, &res);
	debug2("T11 IS    P: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Sh(  80, 180, 1000, e11);
	Dm(-7000, -7000, -400, h, &res);
	debug2("T11 IS   BP: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Zh(-1000, -10,   -1, d11);
	Dm(  100, 400, 700, h, &res);
	debug2("D11 IS    N: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Sh(     1, 10, 1000, d11);
	Dm( -700, -400, -100, h, &res);
	debug2("D11 IS    P: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);

	debug("V11_POS %i %i, V11_SUM %i %i\n", prev->v11_pos, curr->v11_pos,
		       prev->v11_sum, curr->v11_sum);	
	if (prev->v11_pos >=0) {
		curr->v11_pos = prev->v11_pos + res.value;
		if (curr->v11_pos < 0)
			curr->v11_pos = 0;
		else if (curr->v11_sum > 47000)
			curr->v11_pos = 47000;
		curr->v11 = curr->v11_pos - prev->v11_pos;
		if (-50 < curr->v11 && curr->v11 < 50) 
			curr->v11_pos = prev->v11_pos;
	} else {
		curr->v11 = res.value;
		curr->v11_sum = prev->v11_sum;
		if (-50 >= curr->v11 || curr->v11 >= 50) {
			curr->v11_sum += res.value;
			if (curr->v11_sum <= -47000)
				curr->v11_pos = 0;
			else if (47000 <= curr->v11_sum)
				curr->v11_pos = 47000;
		}
	}
}

static void
calculate_m11(struct site_status *curr, struct site_status *prev)
{
	struct action *action = (void*) xmalloc(sizeof(*action));
	action->type = DIGITAL_OUTPUT;
	action->data.digital.delay = 0;
	action->data.digital.value = 0x00000000;
	action->data.digital.mask = 0x00000000;

	curr->m11_fail = prev->m11_fail;
	if ((curr->do0 & 0x00000004) == 0) {
		action->data.digital.value |= 0x00000004;
		action->data.digital.mask |= 0x00000004;
	}
	if (curr->t > 160) {
		if ((curr->do0 & 0x0000000b) != 0)
			action->data.digital.mask |= 0x0000000b;
		queue_action(action);
		curr->m11_int = 0;
		return;
	}

	if (curr->t > 140 && curr->do0 & 0x3 == 0) {
		queue_action(action);
		return;
	}

	curr->m11_int = prev->m11_int + 1;

	if (!curr->m11_fail && curr->m11_int > 10
			&& (curr->p11 - curr->p12 < 40)
			&& (prev->p11 - prev->p12 < 40)) {
		curr->m11_fail = curr->do0 & 0x3;
		action->data.digital.value |= 0x00000003;
		action->data.digital.mask |= 0x00000003;
		curr->m11_int = 0;
	}

	if ((curr->do0 & 0x00000008) == 0) {
		action->data.digital.value |= 0x00000008;
		action->data.digital.mask |= 0x00000008;
	}
	if (curr->m11_fail) {
		queue_action(action);
		return;
	}

	if ((curr->do0 & 0x00000003) == 0) {
		action->data.digital.value |= 1 << (curr->t % 2);
		action->data.digital.mask |= 0x00000003;
		curr->m11_int = 0;
	} else if (curr->m11_int > 7 * 24 * 60 * 6) {
		action->data.digital.value |= (curr->do0 ^ 0x3) & 0x3;
		action->data.digital.mask |= 0x00000003;
		curr->m11_int = 0;
	}
	queue_action(action);
}

static void
calculate_e2(struct site_status *curr)
{
	curr->e21 = curr->t21 - 570;
}

static void
calculate_v21(struct site_status *curr, struct site_status *prev)
{
	int e21 = curr->e21;
	int d21 = e21 - prev->e21;
	struct fuzzy_result res = {0};
	unsigned h;

	h =  Zh(-1000,-50,  -30, e21);
	Dm(  400, 5000, 5000, h, &res);
	debug2("T21 IS   BN: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh( -50, -30,  -10, e21);
	Dm(  100, 400, 700, h, &res);
	debug2("T21 IS    N: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh( -30, -10,    0, e21);
	Dm(    0, 100,  200, h, &res);
	debug2("T21 IS   SN: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh( -10,   0,   30, e21);
	Dm( -300,   0,  300, h, &res);
	debug2("T21 IS Zero: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh(   0,  30,   80, e21);
	Dm( -200, -100, 0, h, &res);
	debug2("T21 IS   SP: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh(  30,  80,  180, e21);
	Dm( -700, -400, -100, h, &res);
	debug2("T21 IS    P: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Sh(  80, 180, 1000, e21);
	Dm(-5000, -5000, -400, h, &res);
	debug2("T21 IS   BP: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Zh(-1000, -10,   -1, d21);
	Dm(  100, 400, 700, h, &res);
	debug2("D21 IS    N: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Sh(     1, 10, 1000, d21);
	Dm( -700, -400, -100, h, &res);
	debug2("D21 IS    P: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);

	curr->v21 = res.value;
}

static unsigned
execute_v11(struct site_status *curr)
{
	struct action action = {0};
	long usec;

	action.type = DIGITAL_OUTPUT;

	if (-50 < curr->v11 && curr->v11 < 50)
		return 0;

	action.data.digital.mask |= 0x00000030;
	if (curr->v11 < 0) {
		usec = curr->v11 * -1000;
		action.data.digital.value = 0x00000010;
	} else {
		usec = curr->v11 * 1000;
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

static unsigned
execute_v21(struct site_status *curr)
{
	struct action action = {0};
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

static void
log_status(struct site_status *site_status)
{
	logit("T %3i, T11 %3i, T12 %3i, T21 %3i, P11 %3u, P12 %3u, "
			"V1 %3i, V2 %3i\n", site_status->t,
		       	site_status->t11, site_status->t12, site_status->t21,
		       	site_status->p11, site_status->p12, site_status->v11,
		       	site_status->v21);
}

static void
process_loop(void)
{
	unsigned c = 0;
	unsigned t;
	struct site_status s1 = {0}, s2 = {0};
	struct site_status *curr, *prev;
	struct process *p;
	int err;

	while (load_site_status(&s2) != 0)
		sleep (1);

	s1.v11_pos = -1;
	s1.v21_pos = -1;
	s2.v11_pos = -1;
	s2.v21_pos = -1;
	calculate_e1(&s2);
	calculate_e2(&s2);
	while (!received_sigterm) {
		c = c ^ 1;
		if (c) {
			curr = &s1;
			prev = &s2;
		} else {
			curr = &s2;
			prev = &s1;
		}

		while (load_site_status(curr) != 0)
			sleep (1);

		list_for_each_entry(p, &process_list, process_entry) {
			p->run(curr, prev, p->process_config);
		}
		calculate_e1(curr);
		calculate_v11(curr, prev);
		calculate_e2(curr);
		calculate_v21(curr, prev);
		calculate_m11(curr, prev);
		log_status(curr);
		t = 10000000;
		t -= execute_v11(curr);;
		t -= execute_v21(curr);;
		t -= execute_actions(curr);
		usleep(t);
	}
}

static void
usage(void)
{
	fprintf(stderr, "%s, version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	fprintf(stderr,
"usage: pcs [-D] [-f config_file]\n"
	);
	exit(1);
}

int
main(int ac, char **av)
{
	FILE *f;
	int opt;
	int log_level = LOG_NOTICE;

	while ((opt = getopt(ac, av, "f:dDt")) != -1) {
		switch (opt) {
                case 'f': 
			config_file_name = optarg;
			break;
		case 'd':
			log_level = LOG_DEBUG;
			break;
		case 'D':
			no_detach_flag = 1;
			break;
		case 't':
			return 0;
		case '?':
		default:
			usage();
			break;
		}
	}

	if (!no_detach_flag)
		daemon(0, 0);

	log_init("pcs", log_level, LOG_DAEMON, no_detach_flag);
	signal(SIGTERM, sigterm_handler);
	signal(SIGQUIT, sigterm_handler);
	signal(SIGINT, sigterm_handler);
	f = fopen(pid_file, "w");
	if (f != NULL) {
		fprintf(f, "%ld\n", getpid());
		fclose(f);
	}

	process_loop();

	if (!no_detach_flag)
		closelog();
	unlink(pid_file);
	return 0;
}
