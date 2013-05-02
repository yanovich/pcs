/* fuzzy.h -- calculate process adjustment with fuzzy logic
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

struct fuzzy_result {
	unsigned	mass;
	int		value;
};

unsigned
Dh(int a, int b, int c, int x);

void
Dm(int a, int b, int c, unsigned h, struct fuzzy_result *result);

unsigned
Sh(int a, int b, int c, int x);

unsigned
Zh(int a, int b, int c, int x);

