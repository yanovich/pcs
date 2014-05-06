/* block-list.h -- available process blocks
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

#ifndef _PCS_BLOCK_LIST_H
#define _PCS_BLOCK_LIST_H

#include "const.h"
#include "i-8042.h"
#include "i-87015.h"
#include "fuzzy-if-d.h"
#include "fuzzy-if-s.h"
#include "fuzzy-if-z.h"
#include "fuzzy-then-d.h"
#include "logger.h"
#include "map.h"
#include "ni1000tk5000.h"
#include "pd.h"

static struct pcs_map loaders[] = {
	{
		.key		= "const",
		.value		= load_const_builder,
	}
	,{
		.key		= "fuzzy if d",
		.value		= load_fuzzy_if_d_builder,
	}
	,{
		.key		= "fuzzy if s",
		.value		= load_fuzzy_if_s_builder,
	}
	,{
		.key		= "fuzzy if z",
		.value		= load_fuzzy_if_z_builder,
	}
	,{
		.key		= "fuzzy then d",
		.value		= load_fuzzy_then_d_builder,
	}
	,{
		.key		= "i-8042",
		.value		= load_i_8042_builder,
	}
	,{
		.key		= "i-87015",
		.value		= load_i_87015_builder,
	}
	,{
		.key		= "log",
		.value		= load_log_builder,
	}
	,{
		.key		= "ni1000tk5000",
		.value		= load_ni1000tk5000_builder,
	}
	,{
		.key		= "PD",
		.value		= load_pd_builder,
	}
	,{
	}
};

#endif
