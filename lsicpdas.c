/* vim:set ts=8 sw=8 sts=8 cindent tw=79 ft=c: */
/* lsicpdas.c -- list present ICP DAS I-8(7) modules
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
#define REQUEST_NAME	"$00M"

int main(int argc, char *argv[])
{
	int err;
	char buff[MAX_RESPONSE];
	int slot_count, slot;

	log_init("lsicpdas", LOG_NOTICE, LOG_DAEMON, 1);

	slot_count = icp_module_count();
	if (slot_count < 0)
		return 1;

	for (slot = 1; slot <= slot_count; slot++) {
		char *name = buff;
		if (icp_get_parallel_name(slot, MAX_RESPONSE - 1, name) != 0) {
			if (icp_serial_exchange(slot, REQUEST_NAME,
					       	MAX_RESPONSE - 1, name) < 3)
				strcpy(name, "empty");
			else
				name = &buff[3];
		}
		printf("slot%i..%s\n", slot, name);
	}

	return 0;
}
