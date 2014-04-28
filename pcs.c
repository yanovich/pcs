/* pcs.c -- process control service
 * Copyright (C) 2014 Sergei Ianovich <ynvich@gmail.com>
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

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "list.h"

struct server_config {
	struct timeval		tick;
	struct list_head	block_list;
};

struct server_state {
	struct timeval		start;
};

struct block {
	struct list_head	block_entry;
	struct block_ops	*ops;
};

struct block_ops {
	void	(*run)(struct block *b, struct server_state *s);
};

void
test_run(struct block *b, struct server_state *s)
{
	char buff[24];
	struct tm tm = *localtime(&s->start.tv_sec);
	strftime(&buff[0], sizeof(buff) - 1, "%b %e %H:%M:%S", &tm);
	printf("%s Greetings, world!\n", buff);
	usleep(100000);
}

struct block_ops test_ops = {
	.run		= test_run,
};

struct block test = {
	.ops		= &test_ops,
};

void
load_server_config(const char const *filename, struct server_config *conf)
{
	conf->tick.tv_sec = 1;
	conf->tick.tv_usec = 0;
	list_add(&test.block_entry, &conf->block_list);
}

void
next_tick(struct server_state *s)
{
	struct timeval now;
	long delay;

	gettimeofday(&now, NULL);
	delay = s->start.tv_usec - now.tv_usec +
		1000000 * (s->start.tv_sec - now.tv_sec);
	if (0 >= delay) {
		printf("missed tick by %li usec\n", -delay);
		return;
	}
	usleep(delay);
}

int main()
{
	struct server_config c;
	struct server_state s;
	struct block *b;

	INIT_LIST_HEAD(&c.block_list);
	load_server_config(NULL, &c);
	gettimeofday(&s.start, NULL);

	while (1) {
		list_for_each_entry(b, &c.block_list, block_entry) {
			if (!b || !b->ops || !b->ops->run) {
				printf("bad block 0x%p\n", b);
			}
			b->ops->run(b, &s);
		}
		timeradd(&s.start, &c.tick, &s.start);
		next_tick(&s);
	}

	return 0;
}
