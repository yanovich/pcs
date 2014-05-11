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

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
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
	const char *pid_file = PIDDIR "/pcs.pid";
	int log_level = LOG_NOTICE;
	int test_only = 0;
	struct server_config c = {
		.tick		= {0},
		.multiple	= 1,
       	};
	struct server_state s;
	struct block *b;
	int opt;
	int no_detach = 0;
	FILE *f;

	while ((opt = getopt(argc, argv, "Ddf:t")) != -1) {
		switch (opt) {
		case 'D':
			no_detach = 1;
			break;
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
	s.tick = c.tick;

	if (!no_detach)
		daemon(0, 0);

	log_init("pcs", log_level, LOG_DAEMON, no_detach);

        f = fopen(pid_file, "w");
        if (f != NULL) {
                fprintf(f, "%lu\n", (long unsigned) getpid());
                fclose(f);
        }

	while (1) {
		char buff[24];
		struct tm tm = *localtime(&s.start.tv_sec);
		strftime(&buff[0], sizeof(buff) - 1, "%b %e %H:%M:%S", &tm);
		debug("%s\n", buff);

		list_for_each_entry(b, &c.block_list, block_entry) {
			if (--b->counter)
				continue;
			b->counter = b->multiple;
			b->ops->run(b, &s);
		}
		timeradd(&s.start, &c.tick, &s.start);
		next_tick(&s);
	}

	if (!no_detach)
		closelog();

	unlink(pid_file);

	return 0;
}
