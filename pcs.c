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

int main(int argc, char **argv)
{
	const char *config_file_name = SYSCONFDIR "/pcs.conf";
	int log_level = LOG_NOTICE;
	int test_only = 0;
	struct server_config c;
	struct server_state s;
	struct block *b;
	int opt;

	while ((opt = getopt(argc, argv, "df:t")) != -1) {
		switch (opt) {
		case 'd':
			if (log_level >= LOG_DEBUG)
				log_level++;
			else
				log_level = LOG_DEBUG;
			break;
		case 'f':
			config_file_name = optarg;
			break;
		case 't':
			test_only = 1;
			break;
		default:
			break;
		}
	}

	INIT_LIST_HEAD(&c.block_list);
	log_init("pcs", log_level, LOG_DAEMON, 1);
	load_server_config(config_file_name, &c);
	if (&c.block_list == c.block_list.next)
		fatal("Nothing to do. Exiting\n");
	if (test_only)
		return 0;
	gettimeofday(&s.start, NULL);

	while (1) {
		list_for_each_entry(b, &c.block_list, block_entry) {
			b->ops->run(b, &s);
		}
		timeradd(&s.start, &c.tick, &s.start);
		next_tick(&s);
	}

	return 0;
}
