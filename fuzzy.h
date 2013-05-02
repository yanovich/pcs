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

#include "list.h"

struct fuzzy_clause {
	struct list_head	fuzzy_entry;
	int			var;
	int			h_f;
	int			h_a;
	int			h_b;
	int			h_c;
	int			m_f;
	int			m_a;
	int			m_b;
	int			m_c;
};

static inline void
fuzzy_clause_init(struct fuzzy_clause *fcl, int var, int h_f, int h_a, int h_b,
	       	int h_c, int m_f, int m_a, int m_b, int m_c)
{
	fcl->var = var;
	fcl->h_f = h_f;
	fcl->h_a = h_a;
	fcl->h_b = h_b;
	fcl->h_c = h_c;
	fcl->m_f = m_f;
	fcl->m_a = m_a;
	fcl->m_b = m_b;
	fcl->m_c = m_c;
}

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

int
process_fuzzy(struct list_head *fuzzy, int *vars);
