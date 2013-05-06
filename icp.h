/* icp.h -- exchange data with ICP DAS I-8(7) modules
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

#ifndef ICP_H
#define ICP_H

enum {
	DO_MODULE,
	DI_MODULE,
	TR_MODULE,
	NULL_MODULE_TYPE
};

int
icp_serial_exchange(unsigned int slot, const char *cmd, int size, char *data);

int
icp_module_count(void);

int
icp_get_parallel_name(unsigned int slot, int size, char *data);

void
icp_list_modules(void (*callback)(unsigned int, const char *));

void *
icp_init_module(const char *name, int n, int *type, int *more);

#endif /* ICP_H */
