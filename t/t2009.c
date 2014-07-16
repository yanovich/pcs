/* t/t2009.c -- test central heating control block
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

#include "includes.h"

#include <unistd.h>

#include "block.h"
#include "central-heating.h"
#include "list.h"
#include "map.h"
#include "serverconf.h"
#include "state.h"

#define PCS_TEST_OBJECT "central heating"

int main(int argc, char **argv)
{
	struct server_state s;
	struct block_builder *bb;
	struct block *b;
	void (*set_input)(void *, const char const *, long *);
	long flowback, street, res;
	long feed_setpoint, flowback_setpoint, street_setpoint;
	long inside_setpoint, stop_setpoint;

	log_init("t2009", LOG_DEBUG + 2, LOG_DAEMON, 1);

	bb = load_central_heating_builder();
	b = xzalloc(sizeof(*b));
	b->data = bb->alloc();
	b->ops = bb->ops(b->data);
	if (!b->ops || !b->ops->run)
		fatal("t2009: bad 'central heating' ops\n");

	if (NULL == bb->inputs)
		fatal("t2009: bad 'central heating' input table\n");
	set_input = pcs_lookup(bb->inputs, "flowback");
	if (NULL == set_input)
		fatal("t2009: bad 'central heating.flowback' input\n");
	set_input(b->data, "flowback", &flowback);

	set_input = pcs_lookup(bb->inputs, "street");
	if (NULL == set_input)
		fatal("t2009: bad 'central heating.street' input\n");
	set_input(b->data, "street", &street);

	set_input = pcs_lookup(bb->inputs, "street setpoint");
	if (NULL == set_input)
		fatal("t2009: bad 'central heating.street setpoint' input\n");
	set_input(b->data, "street setpoint", &street_setpoint);

	set_input = pcs_lookup(bb->inputs, "feed setpoint");
	if (NULL == set_input)
		fatal("t2009: bad 'central heating.feed setpoint' input\n");
	set_input(b->data, "feed setpoint", &feed_setpoint);

	set_input = pcs_lookup(bb->inputs, "flowback setpoint");
	if (NULL == set_input)
		fatal("t2009: bad 'central heating.flowback setpoint' input\n");
	set_input(b->data, "flowback setpoint", &flowback_setpoint);

	set_input = pcs_lookup(bb->inputs, "inside setpoint");
	if (NULL == set_input)
		fatal("t2009: bad 'central heating.inside setpoint' input\n");
	set_input(b->data, "inside setpoint", &inside_setpoint);

	set_input = pcs_lookup(bb->inputs, "stop setpoint");
	if (NULL == set_input)
		fatal("t2009: bad 'central heating.stop setpoint' input\n");
	set_input(b->data, "stop setpoint", &stop_setpoint);

	if (NULL != bb->inputs[7].key)
		fatal("t2009: bad 'central heating' input count\n");

	if (NULL == bb->outputs)
		fatal("t2009: bad 'central heating' output table\n");
	if (NULL != bb->outputs[0])
		fatal("t2009: bad 'central heating' output count\n");
	b->outputs = &res;

	if (NULL != bb->setpoints)
		fatal("t2009: bad 'central heating' setpoints table\n");

	feed_setpoint = 950;
	flowback_setpoint = 700;
	street_setpoint = -280;
	inside_setpoint = 230;
	stop_setpoint = 120;

	flowback = 700;
	street = -280;
	res = 0;
	b->ops->run(b, &s);

	if (res != 950)
		fatal("t2009: bad '%s' result for %li/%li (%li)\n",
				PCS_TEST_OBJECT, street, flowback, res);

	flowback = 720;
	street = -280;
	res = 0;
	b->ops->run(b, &s);

	if (res != 930)
		fatal("t2009: bad '%s' result for %li/%li (%li)\n",
				PCS_TEST_OBJECT, street, flowback, res);
	flowback = 400;
	street = -40;
	res = 0;
	b->ops->run(b, &s);

	if (res != 690)
		fatal("t2009: bad '%s' result for %li/%li (%li)\n",
				PCS_TEST_OBJECT, street, flowback, res);
	return 0;
}
