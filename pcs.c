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
#include <stdlib.h>

#include "includes.h"
#include "pathnames.h"
#include "fuzzy.h"
#include "icp.h"
#include "log.h"
#include "list.h"
#include "process.h"

const char *pid_file = PCS_PID_FILE_PATH;
const char *config_file_name = PCS_CONF_FILE_PATH;

int no_detach_flag = 0;

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
	int test_config = 0;

	while ((opt = getopt(ac, av, "f:dDth")) != -1) {
		switch (opt) {
                case 'f': 
			config_file_name = optarg;
			break;
		case 'd':
			if (log_level >= LOG_DEBUG)
				log_level++;
			else
				log_level = LOG_DEBUG;
			break;
		case 'D':
			no_detach_flag = 1;
			break;
		case 't':
			test_config = 1;
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}

	log_init("pcs", log_level, LOG_DAEMON, 1);
	load_site_config(config_file_name);
	if (test_config)
		return 0;

	if (!no_detach_flag)
		daemon(0, 0);

	log_init("pcs", log_level, LOG_DAEMON, no_detach_flag);
	f = fopen(pid_file, "w");
	if (f != NULL) {
		fprintf(f, "%lu\n", (long unsigned) getpid());
		fclose(f);
	}

	process_loop();

	if (!no_detach_flag)
		closelog();
	unlink(pid_file);
	return 0;
}
