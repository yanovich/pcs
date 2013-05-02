/* fuzzy.c -- calculate process adjustment with fuzzy logic
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

#include <stdlib.h>

#include "fuzzy.h"

unsigned
Dh(int a, int b, int c, int x)
{
	if (x < a || x > c || a > b || b > c)
		return 0;
	if (x == b)
		return 0x10000;
	if (a <= x && x < b)
		return (0x10000 * (x - a)) / (b - a);
	else
		return (0x10000 * (c - x)) / (c - b);
}

void
Dm(int a, int b, int c, unsigned h, struct fuzzy_result *result)
{
	unsigned mass;
	int value;

	if (a > b || b > c)
		return;
	mass =  (h * (unsigned) (c - a)) / 0x20000;
	value = (c + a) / 2 + ((((int) h) * (2 * b - c - a)) / (int) 0x60000);
	value -= result->value;
	result->mass += mass;
	if (result->mass != 0)
		result->value += (value * (int) mass) / (int) result->mass;
}

unsigned
Sh(int a, int b, int c, int x)
{
	if (x < a || x > c || a > b || b > c)
		return 0;
	if (x >= b)
		return 0x10000;

	return (0x10000 * (x - a)) / (b - a);
}

unsigned
Zh(int a, int b, int c, int x)
{
	if (x < a || x > c || a > b || b > c)
		return 0;
	if (x <= b)
		return 0x10000;

	return (0x10000 * (c - x)) / (c - b);
}

unsigned (*ha[])(int, int, int, int) = {Dh, Sh, Zh};
#define HEIGHT_FUNCS	(sizeof(h)/sizeof(void*))
void (*ma[])(int, int, int, unsigned, struct fuzzy_result *) = {Dm};
#define MASS_FUNCS	(sizeof(m)/sizeof(void*))

int
process_fuzzy(struct list_head *fuzzy, int *vars)
{
	struct fuzzy_clause *f;
	struct fuzzy_result r = {0};
	unsigned h;
	list_for_each_entry(f, fuzzy, fuzzy_entry) {
		h = ha[f->h_f](f->h_a, f->h_b, f->h_c, vars[f->var]);
		ma[f->m_f](f->m_a, f->m_b, f->m_c, h, &r);
	}
	return r.value;
}
