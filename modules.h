/* modules.h -- configure modules
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


#ifndef PCS_MODULES_H
#define PCS_MODULES_H

struct DO_mod {
	int		(*read)(struct DO_mod *mod);
	int		(*write)(struct DO_mod *mod);
	unsigned int	count;
	unsigned int	state;
	int		block;
	int		slot;
};

struct TR_mod {
	int		(*read)(struct TR_mod *mod);
	unsigned int	count;
	unsigned int	first;
	int		block;
	int		slot;
};

struct TR_sensor {
	int		t;
	int		(*convert)(int ohms);		
	int		mod;
};

#endif /* PCS_MODULES_H */
