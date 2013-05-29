/* fc-raw.c -- exchange data with Danfoss AquaDrive FC
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

#include "log.h"

int
fc_serial_exchange(const char *cmd, int size, char *data)
{
	int fd, err;
	struct termios options;
	char buff[4];
	int i = 0;

	fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY);
	if (-1 == fd) {
		error("%s:%u: %s (E%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}

	tcgetattr(fd, &options);

	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);

	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_cflag |= PARENB;
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
	options.c_cc[VTIME] = 10;
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
		error("%s:%u: %s (E%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		goto close_fd;
	}

	err = write(fd, cmd, cmd[1] + 2);
	if(0 > err) {
		error("%s:%u: %s (E%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		goto close_fd;
	}

	while (1) {
		err = read(fd, buff, 1);
		if(0 > err) {
			error("%s:%u: %s (E%i)\n", __FILE__, __LINE__,
				       	strerror(errno), errno);
			goto close_fd;
		} else if (0 == err) {
			break;
		}
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
close_fd:
	close(fd);
	return err;
}

static void usage(void)
{
	fprintf(stderr, "Usage: fc-raw param\n");
}

#define MAX_RESPONSE		256

int main(int argc, char *argv[] )
{
	int err;
	int i;
	char data[MAX_RESPONSE];
	char cmd[MAX_RESPONSE];
	char *bad;
	unsigned long param;

	if (argc < 2) {
		usage();
		return 1;
	}

	param = strtoul(argv[1], &bad, 0);
	if (bad[0] != 0) {
		usage();
		return 1;
	}

	cmd[0] = 0x02;
	cmd[1] =   14;
	cmd[2] = 0x01;
	/* PKE */
	cmd[3] = 0x10 | ((param >> 8) & 0x3f);
	cmd[4] = param & 0xff;
	/* IND */
	cmd[5] = 0x00;
	cmd[6] = 0x00;
	/* PWE1 */
	cmd[7] = 0x00;
	cmd[8] = 0x00;
	/* PWE2 */
	cmd[9] = 0x00;
	cmd[10] = 0x00;
	/* PCD1 */
	cmd[11] = 0x00;
	cmd[12] = 0x00;
	/* PCD2 */
	cmd[13] = 0x00;
	cmd[14] = 0x00;
	/* BCC */
	cmd[15] = 0x00;
	printf("S: ");
	for (i = 0; i < 15; i++) {
		printf("%02x", cmd[i]);
		cmd[15] ^= cmd[i];
	}
	printf("%02x\n", cmd[15]);
	err = fc_serial_exchange(cmd, MAX_RESPONSE - 1, data);
	if (err < 0) {
		error ("fc: exchage failed\n");
		return 1;
	}

	printf("R: ");
	for (i = 0; i < err; i++) {
		printf("%02x", data[i]);
	}
	printf("\n");
	return 0;
}
