#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

int get_serial_raw_data(unsigned int slot, const char *cmd, int size,
	       	char *data)
{
	int fd, active_port_fd, err;
	struct termios options;
	char buff[4];
	int i = 0;

	if (slot == 0 || slot > 8) {
		printf("%s %u: bad slot (%u)\n", __FILE__, __LINE__, slot);
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

int get_parallel_output_status(unsigned int slot, unsigned *status)
{
	int fd, err;
	struct termios options;
	char buff[256];
	char *p;
	int i = 0;

	if (slot == 0 || slot > 8) {
		printf("%s %u: bad slot (%u)\n", __FILE__, __LINE__, slot);
		return -1;
	}
	err = snprintf(&buff[0], sizeof(buff) - 1,
		       	"/sys/bus/icpdas/devices/slot%02u/output_status", slot);
	if (err >= sizeof(buff)) {
		printf("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	fd = open(buff, O_RDWR);
	if (-1 == fd) {
		printf("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}

	err = read(fd, buff, 256);
	if(0 > err) {
		printf("%s %u: %s (%i)\n", __FILE__, __LINE__,
				strerror(errno), errno);
		goto close_fd;
	}

	err = 0;
	*status = strtoul(buff, &p, 16);
close_fd:
	close(fd);
	return err;
}

int ni1000(int ohm)
{
	const int o[] = {7909, 8308, 8717, 9135 , 9562 , 10000 ,
	       	10448 , 10907 , 11140 , 11376 , 11857 , 12350 , 12854 ,
	       	13371 , 13901 , 14444 , 15000 , 15570 , 16154 , 16752 ,
	       	17365 , 17993 };
	int i = sizeof(o) / (2 * sizeof(*o));
	int l = (sizeof(o) / sizeof (*o)) - 2;
	int f = 0;
	do {
		if (o[i] <= ohm)
			if (o[i + 1] > ohm) {
				return (100 * (ohm - o[i])) /
					(o[i + 1] - o[i]) + i * 100 -  500;
			}
			else if (i == l)
				return 0x80000000;
			else {
				f = i;
				i = i + 1 + (l - i - 1) / 2;
			}
		else
			if (i == f)
				return 0x80000000;
			else {
				l = i;
				i = i - 1 - (i - 1 - f) / 2;
			}
	} while (1);
}

int b016(int apm)
{
	if (apm < 3937 || apm > 19685)
		return 0x80000000;
	return (1600 * (apm - 3937)) / (19685 - 3937);
}

int parse_amps(const char *data, int size, int *press)
{
	int i = 0, j, point = 0, val = 0;
	const char *p = data;

	if (*p != '+')
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
		} else if ('+' == *p || 0 == *p) {
			if (i >= size)
				return size + 1;
			press[i++] = b016(val);
			if (!*p)
				return i;
			val = 0;
			point = 0;
		}
		p++;
	}
}

int parse_ohms(const char *data, int size, int *temp)
{
	int i = 0, j, point = 0, val = 0;
	const char *p = data;

	if (*p != '+')
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
		} else if ('+' == *p || 0 == *p) {
			if (i >= size)
				return size + 1;
			temp[i++] = ni1000(val);
			if (!*p)
				return i;
			val = 0;
			point = 0;
		}
		p++;
	}
}

struct site_status {
	int		t;
	int		t11;
	int		t12;
	int		t21;
	unsigned int	p11;
	unsigned int	p12;
	int		v11;
	int		v21;
};

int load_site_status(struct site_status *site_status)
{
	int err;
	char data[256];
	int temp[7] = {0};
	int press[8] = {0};
	unsigned output;

	err = get_serial_raw_data(3, "#00", 256, &data[0]);
	if (0 > err) {
		printf("%s: temp data read failure\n", __FILE__);
		return err;
	}
	if ('>' != data[0]) {
		printf("%s: bad temp data: %s\n", __FILE__, data);
		return -1;
	}
	err = parse_ohms(&data[1], sizeof(temp), temp);
	if (7 != err) {
		printf("%s: bad temp data: %s\n", __FILE__, data);
		return -1;
	}
	site_status->t		= temp[4];
	site_status->t11	= temp[0];
	site_status->t12	= temp[1];
	site_status->t21	= temp[3];

	err = get_serial_raw_data(4, "#00", 256, &data[0]);
	if (0 > err) {
		printf("%s: pressure data read failure\n", __FILE__);
		return err;
	}

	err = parse_amps(&data[1], sizeof(press), press);
	if (8 != err) {
		printf("%s: bad temp data: %s(%i)\n", __FILE__, data, err);
		return -1;
	}
	site_status->p12 = press[0];
	site_status->p11 = press[1];

	err = get_parallel_output_status(1, &output);
	if (0 != err) {
		printf("%s: status data\n", __FILE__);
		return -1;
	}

	site_status->v11 = 0;
	site_status->v21 = 0;
	if (output & 0x20)
		site_status->v11 = 1;
	if (output & 0x10)
		site_status->v11 = -1;
	if (output & 0x40)
		site_status->v21 = -1;
	if (output & 0x80)
		site_status->v21 = 1;
	return 0;
};

void
run(void)
{
	char buff[24];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	struct site_status site_status = {0};

	strftime(&buff[0], sizeof(buff) - 1, "%b %e %H:%M:%S", &tm);
	load_site_status(&site_status);
	printf("%s T %3i, T11 %2i, T12 %2i, T21 %2i, P11 %4u, P12 %4u, "
			"V11 %2i, V12 %2i\n", buff, site_status.t,
		       	site_status.t11, site_status.t12, site_status.t21,
		       	site_status.p11, site_status.p12, site_status.v11,
		       	site_status.v21);
}

int
main(int argc, char **argv)
{
	while (1) {
		run();
		sleep(1);
	}
	return 0;
}
