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
#include "fc_type.h"

int
fc_serial_exchange(const char *cmd, int size, char *data)
{
	int fd, err;
	struct termios options;
	int i = 0;
	char crc = 0;
	int n, t = 50;

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

	while (i < 3) {
		err = read(fd, &data[i], 3 - i);
		if(0 > err) {
			error("%s:%u: %s (E%i)\n", __FILE__, __LINE__,
				       	strerror(errno), errno);
			goto close_fd;
		} else if (0 == err) {
			printf("t %i\n", t);
			if (--t <= 0) {
				error("%s:%u: timed out\n",
					       	__FILE__, __LINE__);
				err = -1;
				goto close_fd;
			}
		}
		i += err;
	}

	if (data[0] != 0x02) {
		error("%s:%u: bad response 0x%02x, expected 0x02\n",
			       	__FILE__, __LINE__, data[0]);
		err = -1;
		goto close_fd;
	}

	n = 2 + data[1];
	if (n >= size) {
		error("%s:%u: response is too long (%i), expected %i\n",
			       	__FILE__, __LINE__, n, size);
		err = 1;
		goto close_fd;
	}

	if (data[2] != cmd[2]) {
		error("%s:%u: bad address 0x%02x, expected 0x%02x\n",
			       	__FILE__, __LINE__, data[2], cmd[2]);
		err = 2;
		goto close_fd;
	}

	while (i < n) {
		err = read(fd, &data[i], n - i);
		if(0 > err) {
			error("%s:%u: %s (E%i)\n", __FILE__, __LINE__,
				       	strerror(errno), errno);
			goto close_fd;
		} else if (0 == err) {
			if (--t <= 0) {
				error("%s:%u: timed out\n",
					       	__FILE__, __LINE__);
				err = -1;
				goto close_fd;
			}
		}
		i += err;
	}

	n--;
	for (i = 0; i < n; i++)
		crc ^= data[i];
	if (data[i] != crc) {
		error("%s:%u: bad response CRC (0x%02x), expected 0x%02x\n",
			       	__FILE__, __LINE__, data[i], crc);
		err = 3;
		goto close_fd;
	}
	err = i + 1;
close_fd:
	close(fd);
	return err;
}

static void usage(int ret)
{
	fprintf(stderr, "Usage:\n"
" fc-raw [-c CTW] [-p param [-i index] [-w write_value]]\n");
	exit(ret);
}

#define MAX_RESPONSE		256

static void
fc_status(unsigned long control)
{
	int err;
	int i;
	char data[MAX_RESPONSE];
	char cmd[MAX_RESPONSE];

	cmd[0] = 0x02;
	cmd[1] =    6;
	cmd[2] = 0x01;
	/* PCD1 */
	cmd[3] = (control >> 24) & 0xff;
	cmd[4] = (control >> 16) & 0xff;
	/* PCD2 */
	cmd[5] = (control >> 8) & 0xff;
	cmd[6] = control & 0xff;
	/* BCC */
	cmd[7] = 0x00;
	printf("S: ");
	for (i = 0; i < 7; i++) {
		printf("%02x", cmd[i]);
		cmd[7] ^= cmd[i];
	}
	printf("%02x\n", cmd[7]);
	err = fc_serial_exchange(cmd, MAX_RESPONSE - 1, data);
	if (err < 4) {
		error ("fc: exchage failed\n");
		return;
	}

	printf("R: ");
	for (i = 0; i < err; i++) {
		printf("%02x", data[i]);
	}
	printf("\n");
}

static void
fc_param(unsigned long control, unsigned long param, unsigned long index,
		unsigned long val, int do_write)
{
	int err;
	int i = 0;
	char data[MAX_RESPONSE];
	char cmd[MAX_RESPONSE];
	unsigned flag = 0x10;

	if (do_write) {
		switch (fc_type[param]) {
		case 3:
		case 5:
		case 6:
			flag = 0xe0;
			break;
		case 4:
		case 7:
			flag = 0xd0;
			break;
		default:
			fatal("unsupported param type\n");
		};
	}
	cmd[i++] = 0x02;
	cmd[i++] =   14;
	cmd[i++] = 0x01;
	/* PKE */
	cmd[i++] = flag | ((param >> 8) & 0x3f);
	cmd[i++] = param & 0xff;
	/* IND */
	cmd[i++] = (index >> 8) & 0xff;
	cmd[i++] = index & 0xff;
	/* PWE1 */
	cmd[i++] = (val >> 24) & 0xff;
	cmd[i++] = (val >> 16) & 0xff;
	/* PWE2 */
	cmd[i++] = (val >> 8) & 0xff;
	cmd[i++] = val & 0xff;
	/* PCD1 */
	cmd[i++] = (control >> 24) & 0xff;
	cmd[i++] = (control >> 16) & 0xff;
	/* PCD2 */
	cmd[i++] = (control >> 8) & 0xff;
	cmd[i++] = control & 0xff;
	/* BCC */
	cmd[i++] = 0x00;

	printf("S: ");
	for (i = 0; i < 15; i++) {
		printf("%02x", cmd[i]);
		cmd[15] ^= cmd[i];
	}
	printf("%02x\n", cmd[15]);
	err = fc_serial_exchange(cmd, MAX_RESPONSE - 1, data);
	if (err < 4) {
		error ("fc: exchage failed\n");
		return;
	}

	printf("R: ");
	for (i = 0; i < err; i++) {
		printf("%02x", data[i]);
	}
	printf("\n");
}

static void
fc_text(unsigned long control, unsigned long param, unsigned long index,
		char *val, int do_write)
{
	int err;
	int i = 0;
	int j = 0;
	int n;
	char data[MAX_RESPONSE];
	char cmd[MAX_RESPONSE];

	if (do_write) {
		index += 0x500;
		n = strlen(val);
	} else {
		index += 0x400;
		n = 4;
		val = (char *) &index;
	}

	cmd[i++] = 0x02;
	cmd[i++] = 10 + n;
	cmd[i++] = 0x01;
	/* PKE */
	cmd[i++] = 0xf0 | ((param >> 8) & 0x3f);
	cmd[i++] = param & 0xff;
	/* IND */
	cmd[i++] = (index >> 8) & 0xff;
	cmd[i++] = index & 0xff;
	index = 0;
	/* PWE */
	while (j < n)
		cmd[i++] = val[j++];
	/* PCD1 */
	cmd[i++] = (control >> 24) & 0xff;
	cmd[i++] = (control >> 16) & 0xff;
	/* PCD2 */
	cmd[i++] = (control >> 8) & 0xff;
	cmd[i++] = control & 0xff;
	/* BCC */
	cmd[i] = 0x00;
	n = i;
	printf("S: ");
	for (i = 0; i < n; i++) {
		printf("%02x", cmd[i]);
		cmd[n] ^= cmd[i];
	}
	printf("%02x\n", cmd[n]);
	err = fc_serial_exchange(cmd, MAX_RESPONSE - 1, data);
	if (err < 4) {
		error ("fc: exchage failed\n");
		return;
	}

	printf("R: ");
	for (i = 0; i < err; i++) {
		printf("%02x", data[i]);
	}
	printf("\n");
}

int main(int ac, char *av[] )
{
	int opt;
	char *bad;
	unsigned long control = 0x0;
	unsigned long param = 0;
	unsigned long index = 0;
	unsigned long val = 0;
	char *text = NULL;
	int do_write = 0;
	int log_level = LOG_NOTICE;

	while ((opt = getopt(ac, av, "c:p:i:w:dh")) != -1) {
		switch (opt) {
                case 'c': 
			control = strtoul(optarg, &bad, 0);
			if (bad[0] != 0)
				usage(1);
			break;
		case 'p':
			if (param != 0)
				usage(1);
			param = strtoul(optarg, &bad, 0);
			if (bad[0] != 0)
				usage(1);
			if (param == 0)
				usage(1);
			break;
		case 'i':
			if (param == 0)
				usage(1);
			index = strtoul(optarg, &bad, 0);
			if (bad[0] != 0)
				usage(1);
			break;
		case 'w':
			if (param == 0)
				usage(1);
			if (fc_type[param] == 3 || fc_type[param] == 4)
				val = (unsigned long) strtol(optarg, &bad, 0);
			else if (fc_type[param] >= 5 || fc_type[param] <= 7)
				val = strtoul(optarg, &bad, 0);
			else if (fc_type[param] == 9)
				text = optarg;
			else
				fatal("unsupported param type %i(%lu)\n",
					       fc_type[param], param);
			if (bad[0] != 0)
				usage(1);
			do_write = 1;
			break;
		case 'd':
			if (log_level >= LOG_DEBUG)
				log_level++;
			else
				log_level = LOG_DEBUG;
			break;
		case 'h':
			usage(0);
			break;
		default:
			usage(1);
			break;
		}
	}

	if (param && fc_type[param] == 9)
		fc_text(control, param, index, text, do_write);
	else if (param)
		fc_param(control, param, index, val, do_write);
	else
		fc_status(control);

	return 0;
}
