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

#include "log.h"

int
icp_serial_exchange(unsigned int slot, const char *cmd, int size, char *data)
{
	int fd, active_port_fd, err;
	struct termios options;
	char buff[4];
	int i = 0;

	if (slot == 0 || slot > 8) {
		error("%s %u: bad slot (%u)\n", __FILE__, __LINE__, slot);
		return -1;
	}
	err = snprintf(&buff[0], sizeof(buff) - 1, "%u", slot);
	if (err >= sizeof(buff)) {
		printf("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	fd = open("/dev/ttySA1", O_RDWR | O_NOCTTY);
	if (-1 == fd) {
		printf("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	active_port_fd = open("/sys/bus/icpdas/devices/backplane/active_slot",
			O_RDWR);
	if (-1 == active_port_fd) {
		printf("%u %s\n", __LINE__, strerror(errno));
		err = -1;
		goto close_fd;
	}
	err = write(active_port_fd, buff, 2);
	if (err <= 0) {
		printf("%u %s\n", __LINE__, strerror(errno));
		goto close_fd;
	}
	close(active_port_fd);

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
		printf("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		goto close_fd;
	}

	err = write(fd, cmd, strlen(cmd));
	if(0 > err) {
		printf("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		goto close_fd;
	}

	err = write(fd, "\r", 1);
	if(0 > err) {
		printf("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		goto close_fd;
	}

	while (1) {
		err = read(fd, buff, 1);
		if(0 > err) {
			printf("%s %u: %s (%i)\n", __FILE__, __LINE__,
				       	strerror(errno), errno);
			goto close_fd;
		} else if (0 == err) {
			printf("%s %u: no data\n", __FILE__, __LINE__);
			err = -1;
			goto close_fd;
		}
		if (buff[0] == 13)
			break;
		data[i++] = buff[0];
		if (i == size) {
			printf("%s %u: too much data %i\n", __FILE__, __LINE__,
					i);
			err = -1;
			goto close_fd;
		}
	}

	data[i] = 0;
close_fd:
	close(fd);
	return err;
}

