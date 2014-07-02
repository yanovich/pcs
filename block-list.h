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

#include "cascade.h"
#include "central-heating.h"
#include "const.h"
#include "discrete-valve.h"
#include "i-8041.h"
#include "i-8042.h"
#include "i-87015.h"
#include "i-87017.h"
#include "file-input.h"
#include "file-output.h"
#include "fuzzy-if-d.h"
#include "fuzzy-if-s.h"
#include "fuzzy-if-z.h"
#include "fuzzy-then-d.h"
#include "linear.h"
#include "logger.h"
#include "logical-and.h"
#include "logical-not.h"
#include "logical-or.h"
#include "logical-xor.h"
#include "map.h"
#include "ni1000tk5000.h"
#include "pd.h"
#include "trigger.h"
#include "weighted-sum.h"

static struct pcs_map loaders[] = {
	{
		.key		= "cascade",
		.value		= load_cascade_builder,
	}
	,{
		.key		= "central heating",
		.value		= load_central_heating_builder,
	}
	,{
		.key		= "const",
		.value		= load_const_builder,
	}
	,{
		.key		= "discrete valve",
		.value		= load_discrete_valve_builder,
	}
	,{
		.key		= "file input",
		.value		= load_file_input_builder,
	}
	,{
		.key		= "file output",
		.value		= load_file_output_builder,
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
		.key		= "i-8041",
		.value		= load_i_8041_builder,
	}
	,{
		.key		= "i-8042",
		.value		= load_i_8042_builder,
	}
	,{
		.key		= "i-8042 out",
		.value		= load_i_8042_out_builder,
	}
	,{
		.key		= "i-87015",
		.value		= load_i_87015_builder,
	}
	,{
		.key		= "i-87017",
		.value		= load_i_87017_builder,
	}
	,{
		.key		= "linear",
		.value		= load_linear_builder,
	}
	,{
		.key		= "log",
		.value		= load_log_builder,
	}
	,{
		.key		= "logical AND",
		.value		= load_logical_and_builder,
	}
	,{
		.key		= "logical NOT",
		.value		= load_logical_not_builder,
	}
	,{
		.key		= "logical OR",
		.value		= load_logical_or_builder,
	}
	,{
		.key		= "logical XOR",
		.value		= load_logical_xor_builder,
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
		.key		= "trigger",
		.value		= load_trigger_builder,
	}
	,{
		.key		= "weighted sum",
		.value		= load_weighted_sum_builder,
	}
	,{
	}
};

#endif
