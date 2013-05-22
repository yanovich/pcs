/* icp.c -- exchange data with ICP DAS I-8(7) modules
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

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "pathnames.h"
#include "log.h"
#include "modules.h"
#include "icp.h"

const char *ports[] = {
	"/dev/ttySA1",
	"/dev/ttyS0",
};

int
icp_serial_exchange(unsigned int slot, const char *cmd, int size, char *data)
{
	int fd, active_port_fd, err;
	struct termios options;
	char buff[4];
	int i = 0;
	int serial = slot >> 16;

	if (serial >= (sizeof (ports)/ sizeof(ports[0]))) {
		error("unsupported serial port %i\n", serial);
		return -1;
	}
	slot &= 0xffff;
	if (slot == 0 || slot > 8) {
		error("%s %u: bad slot (%u)\n", __FILE__, __LINE__, slot);
		return -1;
	}
	err = snprintf(&buff[0], sizeof(buff) - 1, "%u", slot);
	if (err >= sizeof(buff)) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	fd = open(ports[serial], O_RDWR | O_NOCTTY);
	if (-1 == fd) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	if (serial == 0) {
		active_port_fd = open(ICP_ACTIVE_SLOT_FILE, O_RDWR);
		if (-1 == active_port_fd) {
			error("%u %s\n", __LINE__, strerror(errno));
			err = -1;
			goto close_fd;
		}
		err = write(active_port_fd, buff, 2);
		if (err <= 0) {
			error("%u %s\n", __LINE__, strerror(errno));
			goto close_fd;
		}
		close(active_port_fd);
	}

	tcgetattr(fd, &options);

	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);

	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~PARODD;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag |= CLOCAL;
	options.c_cflag |= CREAD;
	options.c_cflag &= ~CRTSCTS;

	options.c_iflag &= ~ICRNL;
	options.c_iflag &= ~INLCR;
	options.c_iflag &= ~IXON;
	options.c_iflag &= ~IXOFF;

	options.c_oflag &= ~OPOST;
	options.c_oflag &= ~OLCUC;
	options.c_oflag &= ~ONLCR;
	options.c_oflag &= ~OCRNL;
	options.c_oflag &= ~NLDLY;
	options.c_oflag &= ~CRDLY;
	options.c_oflag &= ~TABDLY;
	options.c_oflag &= ~BSDLY;
	options.c_oflag &= ~VTDLY;
	options.c_oflag &= ~FFDLY;

	options.c_lflag &= ~ISIG;
	options.c_lflag &= ~ICANON;
	options.c_lflag &= ~ECHO;


	options.c_cc[VINTR] = 0;
	options.c_cc[VQUIT] = 0;
	options.c_cc[VERASE] = 0;
	options.c_cc[VKILL] = 0;
	options.c_cc[VEOF] = 4;
	options.c_cc[VTIME] = 1;
	options.c_cc[VMIN] = 0;
	options.c_cc[VSWTC] = 0;
	options.c_cc[VSTART] = 0;
	options.c_cc[VSTOP] = 0;
	options.c_cc[VSUSP] = 0;
	options.c_cc[VEOL] = 0;
	options.c_cc[VREPRINT] = 0;
	options.c_cc[VDISCARD]	= 0;
	options.c_cc[VWERASE] = 0;
	options.c_cc[VLNEXT] =	0;
	options.c_cc[VEOL2] = 0;

	err = tcsetattr(fd, TCSAFLUSH, &options);
	if(0 != err) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		goto close_fd;
	}

	err = write(fd, cmd, strlen(cmd));
	if(0 > err) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		goto close_fd;
	}

	err = write(fd, "\r", 1);
	if(0 > err) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		goto close_fd;
	}

	while (1) {
		err = read(fd, buff, 1);
		if(0 > err) {
			error("%s %u: %s (%i)\n", __FILE__, __LINE__,
				       	strerror(errno), errno);
			goto close_fd;
		} else if (0 == err) {
			err = -1;
			goto close_fd;
		}
		if (buff[0] == 13)
			break;
		data[i++] = buff[0];
		if (i == size) {
			error("%s %u: too much data %i\n", __FILE__, __LINE__,
					i);
			err = -1;
			goto close_fd;
		}
	}

	data[i] = 0;
	err = i;
	debug2("read: [%s]\n", data);
close_fd:
	close(fd);
	return err;
}

int
icp_module_count(void)
{
	int fd;
	char buff[256];
	size_t sz;

	fd = open(ICP_SLOT_COUNT_FILE, O_RDONLY);
	if (-1 == fd) {
		error("%s: %s\n", ICP_SLOT_COUNT_FILE, strerror(errno));
		return -1;
	}
	sz = read(fd, buff, 255);
	close(fd);
	if (0 == sz || 255 <= sz) {
		error("%s:%u: %s\n", __FILE__, __LINE__, strerror(errno));
		return -1;
	}

	return atoi(buff);
}

int
icp_get_parallel_name(unsigned int slot, int size, char *data)
{
	int fd, err;
	char buff[256];
	size_t sz;

	if (slot == 0 || slot > 8) {
		error("%s %u: bad slot (%u)\n", __FILE__, __LINE__, slot);
		return -1;
	}
	err = snprintf(&buff[0], sizeof(buff) - 1,
		       	"/sys/bus/icpdas/devices/slot%02u/model", slot);
	if (err >= sizeof(buff)) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	fd = open(buff, O_RDONLY);
	if (-1 == fd)
		return -1;

	sz = read(fd, data, size);
	close(fd);
	if(0 >= sz) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__,
				strerror(errno), errno);
		return -1;
	}
	data[sz - 1] = 0;

	return 0;
}

static int
get_parallel_input_status(struct DO_mod *module)
{
	int fd, err;
	unsigned int slot = module->slot;
	char buff[256];
	char *p;

	if (slot == 0 || slot > 8) {
		error("%s %u: bad slot (%u)\n", __FILE__, __LINE__, slot);
		return -1;
	}
	err = snprintf(&buff[0], sizeof(buff) - 1,
		       	"/sys/bus/icpdas/devices/slot%02u/input_status", slot);
	if (err >= sizeof(buff)) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	fd = open(buff, O_RDWR);
	if (-1 == fd) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}

	err = read(fd, buff, 256);
	if(0 > err) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__,
				strerror(errno), errno);
		goto close_fd;
	}

	err = 0;
	module->state = strtoul(buff, &p, 16);
close_fd:
	close(fd);
	return err;
}

static int
get_parallel_output_status(struct DO_mod *module)
{
	int fd, err;
	unsigned int slot = module->slot;
	char buff[256];
	char *p;

	if (slot == 0 || slot > 8) {
		error("%s %u: bad slot (%u)\n", __FILE__, __LINE__, slot);
		return -1;
	}
	err = snprintf(&buff[0], sizeof(buff) - 1,
		       	"/sys/bus/icpdas/devices/slot%02u/output_status", slot);
	if (err >= sizeof(buff)) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	fd = open(buff, O_RDWR);
	if (-1 == fd) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}

	err = read(fd, buff, 256);
	if(0 > err) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__,
				strerror(errno), errno);
		goto close_fd;
	}

	err = 0;
	module->state = strtoul(buff, &p, 16);
close_fd:
	close(fd);
	return err;
}

static int
set_parallel_output_status(struct DO_mod *module)
{
	int fd, err;
	unsigned int slot = module->slot;
	char buff[256];

	if (slot == 0 || slot > 8) {
		error("%s %u: bad slot (%u)\n", __FILE__, __LINE__, slot);
		return -1;
	}
	err = snprintf(&buff[0], sizeof(buff) - 1,
		       	"/sys/bus/icpdas/devices/slot%02u/output_status", slot);
	if (err >= sizeof(buff)) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	fd = open(buff, O_RDWR);
	if (-1 == fd) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}

	err = snprintf(&buff[0], sizeof(buff) - 1, "0x%08x", module->state);
	if (err >= sizeof(buff)) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	debug("DO %u is %s\n", slot, buff);
	err = write(fd, buff, strlen(buff));
	if(0 > err) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__,
				strerror(errno), errno);
		goto close_fd;
	}

	err = 0;
close_fd:
	close(fd);
	return err;
}

static int
set_parallel_analog_output(struct AO_mod *module, int port, int value)
{
	int fd, err;
	unsigned int slot = module->slot;
	unsigned int data = ((unsigned int) value) + 0x2000;
	char buff[256];

	if (value < 0 || 0x3fff < value) {
		error("%s:%u: value out of range (0x%08x)\n", __FILE__, __LINE__,
			       	data);
		return -1;
	}
	if (port < 0 || module->count <= port) {
		error("%s:%u: port out of range (%i)\n", __FILE__, __LINE__,
			       	port);
	}
	if (slot == 0 || slot > 8) {
		error("%s %u: bad slot (%u)\n", __FILE__, __LINE__, slot);
		return -1;
	}
	err = snprintf(&buff[0], sizeof(buff) - 1,
		       	"/sys/bus/icpdas/devices/slot%02u/analog_output", slot);
	if (err >= sizeof(buff)) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	fd = open(buff, O_RDWR);
	if (-1 == fd) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}

	data |= ((unsigned int) port) << 14;
	err = snprintf(&buff[0], sizeof(buff) - 1, "0x%04x", data);
	debug("analog output: %u %s\n", slot, buff);
	if (err >= sizeof(buff)) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	err = write(fd, buff, strlen(buff));
	if(0 > err) {
		error("%s %u: %s (%i)\n", __FILE__, __LINE__,
				strerror(errno), errno);
		goto close_fd;
	}

	err = 0;
close_fd:
	close(fd);
	return err;
}

static int
parse_signed_input(const char *data, int size, int *raw)
{
	int i = 0, point = 0, val = 0, sign;
	const char *p = data;

	if ('+' == *p)
		sign = 0;
	else if ('-' == *p)
		sign = 1;
	else
		return 0;

	p++;
	while (1) {
		if (*p >= '0' && *p <= '9') {
			val *= 10;
			val += *p - '0';
		} else if ('.' == *p) {
			if (point)
				return i;
			point = 1;
		} else if ('+' == *p || '-' == *p || 0 == *p) {
			if (i >= size)
				return size + 1;
			if (sign)
				val = -val;
			raw[i++] = val;
			if (!*p)
				return i;
			val = 0;
			point = 0;
			if ('+' == *p)
				sign = 0;
			else
				sign = 1;
		} else {
			return i;
		}
		p++;
	}
}

static int
get_serial_resistance(struct AI_mod *mod, int *buffer)
{
	int err;
	char data[256];

	err = icp_serial_exchange(mod->slot, "#00", 256, &data[0]);
	if (0 > err) {
		error("%s: temp data read failure\n", __FILE__);
		return err;
	}
	if ('>' != data[0]) {
		error("%s: bad temp data: %s\n", __FILE__, data);
		return -1;
	}
	err = parse_signed_input(&data[1], mod->count, buffer);
	if (mod->count != err) {
		error("%s: bad temp data: %s\n", __FILE__, data);
		return -1;
	}
	return 0;
}

static int
get_serial_analog_input(struct AI_mod *mod, int *buffer)
{
	int err;
	char data[256];

	err = icp_serial_exchange(mod->slot, "#00", 256, &data[0]);
	if (0 > err) {
		error("%s: temp data read failure\n", __FILE__);
		return err;
	}
	if ('>' != data[0]) {
		error("%s: bad temp data: %s\n", __FILE__, data);
		return -1;
	}
	err = parse_signed_input(&data[1], mod->count, buffer);
	if (mod->count != err) {
		error("%s: bad temp data: %s\n", __FILE__, data);
		return -1;
	}
	return 0;
}

#define MAX_RESPONSE 256
#define REQUEST_NAME	"$00M"

void
icp_list_modules(void (*callback)(unsigned int, const char *))
{
	char buff[MAX_RESPONSE];
	int slot_count, slot;

	slot_count = icp_module_count();
	if (slot_count < 0)
		return;

	for (slot = 1; slot <= slot_count; slot++) {
		char *name = buff;
		if (icp_get_parallel_name(slot, MAX_RESPONSE - 1, name) != 0) {
			if (icp_serial_exchange(slot, REQUEST_NAME,
					       	MAX_RESPONSE - 1, name) < 3)
				name = NULL;
			else
				name = &buff[3];
		}
		callback(slot, name);
	}
}

static struct AO_mod AO4 = {
	.write = set_parallel_analog_output,
	.count = 4,
};

static struct DO_mod DO16 = {
	.read = get_parallel_output_status,
	.write = set_parallel_output_status,
	.count = 16,
};

static struct DI_mod DI16 = {
	.read = get_parallel_input_status,
	.count = 16,
};

static struct DO_mod DO32 = {
	.read = get_parallel_output_status,
	.write = set_parallel_output_status,
	.count = 32,
};

static struct AI_mod TR7_S = {
	.read = get_serial_resistance,
	.count = 7,
};

static struct AI_mod AI8_S = {
	.read = get_serial_analog_input,
	.count = 8,
}; 

struct module_config {
	const char 		*name;
	int			type;
	void			*config;
};

struct module_config mods[] = {
	{
		.name = "8024",
		.type = AO_MODULE,
		.config = &AO4,
	},
	{
		.name = "8041",
		.type = DO_MODULE,
		.config = &DO32,
	},
	{
		.name = "8042",
		.type = DO_MODULE,
		.config = &DO16,
	},
	{
		.name = "8042",
		.type = DI_MODULE,
		.config = &DI16,
	},
	{
		.name = "87015",
		.type = AI_MODULE,
		.config = &TR7_S,
	},
	{
		.name = "87015P",
		.type = AI_MODULE,
		.config = &TR7_S,
	},
	{
		.name = "87017",
		.type = AI_MODULE,
		.config = &AI8_S,
	},
	{
	}
};

void *
icp_init_module(const char *name, int n, int *type, int *more)
{
	struct module_config *c = NULL;
	int i;

	for (i = 0; i < (sizeof(mods) / sizeof(*c) - 1); i++) {
		if (strcmp(name, mods[i].name))
			continue;
		if (n--)
			continue;
		c = &mods[i];
		*type = mods[i].type;
		if (mods[i + 1].name && !strcmp(name, mods[i + 1].name))
			*more = 1;
		else
			*more = 0;
	}
	if (!c) {
		*type = NULL_MODULE_TYPE;
		return NULL;
	};
	return c->config;
}
