/* dcon-raw.c -- exchange a single command with an DCON module
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
#include <unistd.h>

#include "icpdas.h"

#define MAX_RESPONSE	256

static void
usage(int err)
{
	fprintf(stderr, "Usage: dcon-raw [-d] [-p port] [-s slot] command\n");
	fprintf(stderr, "       dcon-raw  -h\n");
	exit(err);
}

int
main(int argc, char **argv)
{
	const char *device = "/dev/ttyS1";
	int log_level = LOG_NOTICE;
	unsigned int slot = 0;
	char *bad;
	int opt;
	char data[MAX_RESPONSE];
	int err;

	while ((opt = getopt(argc, argv, "dhp:s:")) != -1) {
		switch (opt) {
		case 'd':
			if (log_level >= LOG_DEBUG)
				log_level++;
			else
				log_level = LOG_DEBUG;
			break;
		case 'h':
			usage(0);
			break;
		case 'p':
			device = optarg;
			break;
		case 's':
			slot = (unsigned int) strtoul(optarg, &bad, 10);
			if (bad[0] != 0) {
				fprintf(stderr, "Bad slot value %s\n", optarg);
				exit(1);
			}
			break;
		default:
			usage(1);
			break;
		}
	}

	log_init("icp-raw", log_level, LOG_DAEMON, 1);

	err = icpdas_serial_exchange(device, slot, argv[argc - 1],
			MAX_RESPONSE, data);
	if (0 > err) {
		return 1;
	}

	printf("%s\n", data);

	return 0;
}
