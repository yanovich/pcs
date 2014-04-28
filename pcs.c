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

#include "includes.h"

#include <sys/time.h>
#include <unistd.h>

#include "block.h"
#include "list.h"
#include "serverconf.h"
#include "state.h"

void
next_tick(struct server_state *s)
{
	struct timeval now;
	long delay;

	gettimeofday(&now, NULL);
	delay = s->start.tv_usec - now.tv_usec +
		1000000 * (s->start.tv_sec - now.tv_sec);
	if (0 >= delay) {
		warn("missed tick by %li usec\n", -delay);
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
	log_init("pcs", LOG_NOTICE, LOG_DAEMON, 1);
	load_server_config(NULL, &c);
	gettimeofday(&s.start, NULL);

	while (1) {
		list_for_each_entry(b, &c.block_list, block_entry) {
			if (!b || !b->ops || !b->ops->run) {
				error("bad block 0x%p\n", b);
			}
			b->ops->run(b, &s);
		}
		timeradd(&s.start, &c.tick, &s.start);
		next_tick(&s);
	}

	return 0;
}
