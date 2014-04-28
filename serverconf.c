/* serverconf.c -- process server configuration
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

#include "block.h"
#include "list.h"
#include "serverconf.h"
#include "state.h"

void
mark_run(struct block *b, struct server_state *s)
{
	char buff[24];
	struct tm tm = *localtime(&s->start.tv_sec);
	strftime(&buff[0], sizeof(buff) - 1, "%b %e %H:%M:%S", &tm);
	printf("%s Greetings, world!\n", buff);
	usleep(100000);
}

struct block_ops mark_ops = {
	.run		= mark_run,
};

struct block mark = {
	.ops		= &mark_ops,
};

void
load_server_config(const char const *filename, struct server_config *conf)
{
	conf->tick.tv_sec = 1;
	conf->tick.tv_usec = 0;
	list_add(&mark.block_entry, &conf->block_list);
}
