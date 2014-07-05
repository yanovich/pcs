/* icpdas.h -- exchange data with ICP DAS I-8(7) modules
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

#ifndef _PCS_ICPDAS_H
#define _PCS_ICPDAS_H

void
icpdas_list_modules(void (*callback)(unsigned int, const char *));

int
icpdas_get_parallel_input(unsigned int slot, unsigned long *out);

int
icpdas_get_parallel_output(unsigned int slot, unsigned long *out);

int
icpdas_set_parallel_output(unsigned int slot, unsigned long out);

int
icpdas_set_parallel_analog_output(unsigned int slot, unsigned int port,
		long value);
int
icpdas_get_serial_analog_input(const char const *device, unsigned int slot,
		int size, long *out);
int
icpdas_get_serial_digital_input(const char const *device, unsigned int slot,
		unsigned long *out);
int
icpdas_serial_exchange(const char const *device, unsigned int slot,
		const char const *cmd, int size, char *data);
#endif /* _PCS_ICPDAS_H */
