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

#define MAX_RESPONSE		256

static int
fc_raw_serial_exchange(char *cmd, char *data)
{
	int fd, err;
	struct termios options;
	int i = 0;
	char crc = 0;
	int n, t = 50;
	char buff[MAX_RESPONSE << 1];
	int b;

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

	n = 1 + cmd[1];
	crc = 0;
	b = sprintf(buff, "S: ");
	for (i = 0; i < n; i++) {
		b += sprintf(&buff[b], "%02x", cmd[i]);
		crc ^= cmd[i];
	}
	cmd[n] = crc;
	sprintf(&buff[b], "%02x", cmd[n]);
	debug2("%s\n", buff);

	err = write(fd, cmd, cmd[1] + 2);
	if(0 > err) {
		error("%s:%u: %s (E%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		goto close_fd;
	}

	i = 0;
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
	if (n <= 0) {
		error("%s:%u: bad response length (%i)",
			       	__FILE__, __LINE__, n);
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
	crc = 0;
	b = sprintf(buff, "R: ");
	for (i = 0; i < n; i++) {
		crc ^= data[i];
		b += sprintf(&buff[b], "%02x", data[i]);
	}
	sprintf(&buff[b], "%02x", data[n]);
	debug2("%s\n", buff);

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

struct fc_session {
	unsigned long		control;
	unsigned int		param;
	unsigned int		index;
	union {
		unsigned long	val;
		char		text[64];
	};
	unsigned int			do_write;
};

static int
fc_status(struct fc_session *c, struct fc_session *r)
{
	int err;
	char data[MAX_RESPONSE];
	char cmd[MAX_RESPONSE];
	int i;

	cmd[0] = 0x02;
	cmd[1] =    6;
	cmd[2] = 0x01;
	/* PCD1 */
	cmd[3] = (c->control >> 24) & 0xff;
	cmd[4] = (c->control >> 16) & 0xff;
	/* PCD2 */
	cmd[5] = (c->control >> 8) & 0xff;
	cmd[6] = c->control & 0xff;
	/* BCC */
	cmd[7] = 0x00;

	err = fc_raw_serial_exchange(cmd, data);
	if (err < 4) {
		error ("fc: exchage failed\n");
		return -1;
	}
	r->control = 0;
	for (i = 3; i < 7; i++) {
		r->control <<= 8;
		r->control |= data[i];
	}
	return 0;
}

static int
fc_param(struct fc_session *c, struct fc_session *r)
{
	int err;
	int i = 0;
	char data[MAX_RESPONSE];
	char cmd[MAX_RESPONSE];
	unsigned flag = 0x10;

	if (c->do_write) {
		switch (fc_type[c->param]) {
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
	cmd[i++] = flag | ((c->param >> 8) & 0xf);
	cmd[i++] = c->param & 0xff;
	/* IND */
	cmd[i++] = (c->index >> 8) & 0xff;
	cmd[i++] = c->index & 0xff;
	/* PWE1 */
	cmd[i++] = (c->val >> 24) & 0xff;
	cmd[i++] = (c->val >> 16) & 0xff;
	/* PWE2 */
	cmd[i++] = (c->val >> 8) & 0xff;
	cmd[i++] = c->val & 0xff;
	/* PCD1 */
	cmd[i++] = (c->control >> 24) & 0xff;
	cmd[i++] = (c->control >> 16) & 0xff;
	/* PCD2 */
	cmd[i++] = (c->control >> 8) & 0xff;
	cmd[i++] = c->control & 0xff;
	/* BCC */
	cmd[i++] = 0x00;

	err = fc_raw_serial_exchange(cmd, data);
	if (err < 4) {
		error ("fc: exchage failed\n");
		return -1;
	}
	r->val = 0;
	for (i = 7; i < 11; i++) {
		r->val <<= 8;
		r->val |= data[i];
	}
	r->control = 0;
	for (; i < 15; i++) {
		r->control <<= 8;
		r->control |= data[i];
	}
	r->index = data[6];
	r->param = data[4];
	r->param |= (data[3] & 0xf) << 8;
	flag = data[3] >> 4;
	switch (fc_type[c->param]) {
	case 3:
	case 5:
	case 6:
		if (flag != 1)
			return -1;
		break;
	case 4:
	case 7:
		if (flag != 2)
			return -1;
		break;
	default:
		return -1;
	};
	return 0;
}

static int
fc_text(struct fc_session *c, struct fc_session *r)
{
	int err;
	int i = 0;
	int j = 0;
	int n;
	char data[MAX_RESPONSE];
	char cmd[MAX_RESPONSE];
	unsigned long index = c->index;
	char *text;
	unsigned flag;

	if (c->do_write) {
		index += 0x500;
		n = strlen(c->text);
		text = c->text;
	} else {
		index += 0x400;
		n = 4;
		text = (char *) &index;
	}

	cmd[i++] = 0x02;
	cmd[i++] = 10 + n;
	cmd[i++] = 0x01;
	/* PKE */
	cmd[i++] = 0xf0 | ((c->param >> 8) & 0xf);
	cmd[i++] = c->param & 0xff;
	/* IND */
	cmd[i++] = (index >> 8) & 0xff;
	cmd[i++] = index & 0xff;
	index = 0;
	/* PWE */
	while (j < n)
		cmd[i++] = text[j++];
	/* PCD1 */
	cmd[i++] = (c->control >> 24) & 0xff;
	cmd[i++] = (c->control >> 16) & 0xff;
	/* PCD2 */
	cmd[i++] = (c->control >> 8) & 0xff;
	cmd[i++] = c->control & 0xff;
	/* BCC */
	cmd[i] = 0x00;

	err = fc_raw_serial_exchange(cmd, data);
	if (err < 4) {
		error ("fc: exchage failed\n");
		return -1;
	}
	n = data[1] - 10;
	for (i = 7, j = 0; j < n; i++, j++)
		r->text[j] = data[i];
	r->control = 0;
	n = i + 4;
	for (; i < n; i++) {
		r->control <<= 8;
		r->control |= data[i];
	}
	r->index = data[6];
	r->param = data[4];
	r->param |= (data[3] & 0xf) << 8;
	flag = data[3] >> 4;
	if (flag != 0xf)
		return -1;
	return 0;
}

int
fc_serial_exchange(struct fc_session *cmd, struct fc_session *resp)
{
	int err;

	if (cmd->param && fc_type[cmd->param] == 9) {
		err = fc_text(cmd, resp);
	} else if (cmd->param) {
		err = fc_param(cmd, resp);
	} else {
		err = fc_status(cmd, resp);
	}
	return err;
}

int main(int ac, char *av[] )
{
	int opt;
	char *bad;
	struct fc_session cmd = {0};
	struct fc_session resp = {0};
	int log_level = LOG_NOTICE;

	while ((opt = getopt(ac, av, "c:p:i:w:dh")) != -1) {
		switch (opt) {
                case 'c': 
			cmd.control = strtoul(optarg, &bad, 0);
			if (bad[0] != 0)
				usage(1);
			break;
		case 'p':
			if (cmd.param != 0)
				usage(1);
			cmd.param = strtoul(optarg, &bad, 0);
			if (bad[0] != 0)
				usage(1);
			if (cmd.param == 0)
				usage(1);
			break;
		case 'i':
			if (cmd.param == 0)
				usage(1);
			cmd.index = strtoul(optarg, &bad, 0);
			if (bad[0] != 0)
				usage(1);
			break;
		case 'w':
			if (cmd.param == 0)
				usage(1);
			if (fc_type[cmd.param] == 3
				       	|| fc_type[cmd.param] == 4)
				cmd.val = (unsigned long)
				       	strtol(optarg, &bad, 0);
			else if (fc_type[cmd.param] >= 5
				       	&& fc_type[cmd.param] <= 7)
				cmd.val = strtoul(optarg, &bad, 0);
			else if (fc_type[cmd.param] == 9) {
				if (strlen(optarg) > 63)
					fatal("text is too long\n");
				strcpy(cmd.text, optarg);
			} else
				fatal("unsupported param type %i(%u)\n",
					       fc_type[cmd.param], cmd.param);
			if (bad[0] != 0)
				usage(1);
			cmd.do_write = 1;
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

	log_init(av[0], log_level, LOG_DAEMON, 1);
	if (fc_serial_exchange(&cmd, &resp) < 0)
		return -1;
	printf("Status: 0x%04lx\n", (resp.control >> 16) & 0xffff);

	switch(fc_type[resp.param]) {
	case 3:
		printf("%u-%u[%u]: %i\n", resp.param / 100, resp.param % 100,
				resp.index, (int) (resp.val & 0xffff));
		break;
	case 4:
		printf("%u-%u[%u]: %li\n", resp.param / 100, resp.param % 100,
				resp.index, (long) (resp.val));
		break;
	case 5:
		printf("%u-%u[%u]: %lu\n", resp.param / 100, resp.param % 100,
				resp.index, resp.val & 0xff);
		break;
	case 6:
		printf("%u-%u[%u]: %lu\n", resp.param / 100, resp.param % 100,
				resp.index, resp.val & 0xffff);
		break;
	case 7:
		printf("%u-%u[%u]: %lu\n", resp.param / 100, resp.param % 100,
				resp.index, resp.val);
		break;
	case 9:
		printf("%u-%u[%u]: %s\n", resp.param / 100, resp.param % 100,
				resp.index, resp.text);
		break;
	case 0:
		break;
	}

	return 0;
}
