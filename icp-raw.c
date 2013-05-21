/* vim:set ts=8 sw=8 sts=8 cindent tw=79 ft=c: */
/* icp-raw.c -- exchange a single command with an ICP DAS I-87 module
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "log.h"
#include "icp.h"

#define MAX_RESPONSE 256

static void usage(void)
{
	fprintf(stderr, "Usage: icp-raw port slot command\n");
}

int main(int argc, char *argv[] )
{
	int err;
	char data[MAX_RESPONSE];
	char port;
	char slot;

	if (argc < 4) {
		usage();
		return 1;
	}

	port = argv[1][0] - '0';
	if (argv[1][1] != 0 || port < 0 || port > 1) {
		usage();
		return 1;
	}

	slot = argv[2][0] - '0';
	if (argv[2][1] != 0 || slot < 1 || slot > 8) {
		usage();
		return 1;
	}

	err = icp_serial_exchange((port << 16) | slot, argv[3],
		       	MAX_RESPONSE - 1, data);
	if (err < 0) {
		error ("Exchage failed\n");
		return 1;
	}

	printf("%s\n", data);
	return 0;
}
