/* lsicpdas.c -- list present ICP DAS I-8(7) modules
 * Copyright (C) 2014 Sergey Yanovich <ynvich@gmail.com>
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
#include <string.h>
#include <unistd.h>

#include "icpdas.h"

void print_module(unsigned int slot, const char *name)
{
	printf("slot %i ... %s\n", slot, name ? name : "empty");
}

int main(int argc, char *argv[])
{
	log_init("lsicpdas", LOG_NOTICE, LOG_DAEMON, 1);

	icpdas_list_modules(print_module);

	return 0;
}
