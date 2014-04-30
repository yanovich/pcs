/* map.h -- simple map and tools
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

#ifndef _PCS_MAP_H
#define _PCS_MAP_H

#include <string.h>

struct pcs_map {
	const char		*key;
	void			*value;
};

static inline void *
pcs_lookup(struct pcs_map *map, const char *key)
{
	int i;

	if (!map)
		return NULL;

	for (i = 0; map[i].key; i++) {
		if (strcmp(key, map[i].key))
			continue;
		return map[i].value;
	}

	return NULL;
}
#endif
