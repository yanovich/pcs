#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <syslog.h>

#ifdef DEBUG
#define debug(format, ...) 	printf(format, ...)
#else
#define debug(format, arg...)	\
while (0) {			\
	printf(format, ##arg);	\
}
#endif

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

int set_parallel_output_status(unsigned int slot, unsigned  status)
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

	err = snprintf(&buff[0], sizeof(buff) - 1, "0x%08x", status);
	if (err >= sizeof(buff)) {
		printf("%s %u: %s (%i)\n", __FILE__, __LINE__, strerror(errno),
			       	errno);
		return -1;
	}
	err = write(fd, buff, strlen(buff));
	if(0 > err) {
		printf("%s %u: %s (%i)\n", __FILE__, __LINE__,
				strerror(errno), errno);
		goto close_fd;
	}

	err = 0;
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
	unsigned int	do0;
	int		v11;
	int		v21;
	int		e11;
	int		e12;
	int		e21;
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

	site_status->do0 = output;
	site_status->v11 = 0;
	site_status->v21 = 0;

	return 0;
};

struct fuzzy_result {
	unsigned	mass;
	int		value;
};

unsigned Dh(int a, int b, int c, int x)
{
	if (x < a || x > c || a > b || b > c)
		return 0;
	if (x == b)
		return 0x10000;
	if (a <= x && x < b)
		return (0x10000 * (x - a)) / (b - a);
	else
		return (0x10000 * (c - x)) / (c - b);
}

void Dm(int a, int b, int c, unsigned h, struct fuzzy_result *result)
{
	unsigned mass;
	int value;
	int tmp1, tmp2;

	if (a > b || b > c)
		return;
	mass =  (h * (unsigned) (c - a)) / 0x20000;
	value = (c + a) / 2 + ((((int) h) * (2 * b - c - a)) / (int) 0x60000);
	value -= result->value;
	result->mass += mass;
	if (result->mass != 0)
		result->value += (value * (int) mass) / (int) result->mass;
}

unsigned Sh(int a, int b, int c, int x)
{
	if (x < a || x > c || a > b || b > c)
		return 0;
	if (x >= b)
		return 0x10000;

	return (0x10000 * (x - a)) / (b - a);
}

unsigned Zh(int a, int b, int c, int x)
{
	if (x < a || x > c || a > b || b > c)
		return 0;
	if (x <= b)
		return 0x10000;

	return (0x10000 * (c - x)) / (c - b);
}

void
calculate_e1(struct site_status *curr)
{
	int t11, t12;

	if (curr->t > 160) {
		t12 = curr->t12;
		t11 = 300;
	} else {
		t12 = 700 - ((curr->t + 280) * 250) / 400;
		t11 = 950 - ((curr->t + 280) * 450) / 400;
	}
	curr->e12 = curr->t12 - t12;
	if (curr->e12 > 0)
		t11 -= curr->e12;

	curr->e11 = curr->t11 - t11;
}

void
calculate_v11(struct site_status *curr, struct site_status *prev)
{
	int e11 = curr->e11, d11 = e11 - prev->e11;
	struct fuzzy_result res = {0};
	unsigned h;

	if (curr->e12 > 0)
		d11 -= curr->e12 - prev->e12;

	h =  Zh(-1000,-50,  -30, e11);
	Dm(  400, 5000, 5000, h, &res);
	debug("T11 IS   BN: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh( -50, -30,  -10, e11);
	Dm(  100, 400, 700, h, &res);
	debug("T11 IS    N: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh( -30, -10,    0, e11);
	Dm(    0, 100,  200, h, &res);
	debug("T11 IS   SN: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh( -10,   0,   30, e11);
	Dm( -300,   0,  300, h, &res);
	debug("T11 IS Zero: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh(   0,  30,   80, e11);
	Dm( -200, -100, 0, h, &res);
	debug("T11 IS   SP: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh(  30,  80,  180, e11);
	Dm( -700, -400, -100, h, &res);
	debug("T11 IS    P: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Sh(  80, 180, 1000, e11);
	Dm(-5000, -5000, -400, h, &res);
	debug("T11 IS   BP: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Zh(-1000, -10,   -1, d11);
	Dm(  100, 400, 700, h, &res);
	debug("D11 IS    N: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Sh(     1, 10, 1000, d11);
	Dm( -700, -400, -100, h, &res);
	debug("D11 IS    P: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);

	curr->v11 = res.value;
}

void
calculate_e2(struct site_status *curr)
{
	curr->e21 = curr->t21 - 570;
}

void
calculate_v21(struct site_status *curr, struct site_status *prev)
{
	int e21 = curr->e21;
	int d21 = e21 - prev->e21;
	struct fuzzy_result res = {0};
	unsigned h;

	h =  Zh(-1000,-50,  -30, e21);
	Dm(  400, 5000, 5000, h, &res);
	debug("T21 IS   BN: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh( -50, -30,  -10, e21);
	Dm(  100, 400, 700, h, &res);
	debug("T21 IS    N: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh( -30, -10,    0, e21);
	Dm(    0, 100,  200, h, &res);
	debug("T21 IS   SN: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh( -10,   0,   30, e21);
	Dm( -300,   0,  300, h, &res);
	debug("T21 IS Zero: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh(   0,  30,   80, e21);
	Dm( -200, -100, 0, h, &res);
	debug("T21 IS   SP: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Dh(  30,  80,  180, e21);
	Dm( -700, -400, -100, h, &res);
	debug("T21 IS    P: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Sh(  80, 180, 1000, e21);
	Dm(-5000, -5000, -400, h, &res);
	debug("T21 IS   BP: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Zh(-1000, -10,   -1, d21);
	Dm(  100, 400, 700, h, &res);
	debug("D21 IS    N: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);
	h =  Sh(     1, 10, 1000, d21);
	Dm( -700, -400, -100, h, &res);
	debug("D21 IS    P: 0x%05x, action: %5i, mass: %05x\n", h, res.value, res.mass);

	curr->v21 = res.value;
}

unsigned
execute_v11(struct site_status *curr)
{
	unsigned usec;
	if (-50 < curr->v11 && curr->v11 < 50)
		return 0;

	curr->do0 &= ~0x30;
	if (curr->v11 < 0) {
		usec = curr->v11 * -1000;
		curr->do0 |=  0x10;
	} else {
		usec = curr->v11 *  1000;
		curr->do0 |=  0x20;
	}
	if (usec > 5000000)
		usec = 5000000;

	set_parallel_output_status(1, curr->do0);

	usleep(usec);

	curr->do0 &= ~0x30;
	set_parallel_output_status(1, curr->do0);

	return usec;
}

unsigned
execute_v21(struct site_status *curr)
{
	unsigned usec;
	if (-50 < curr->v21 && curr->v21 < 50)
		return 0;

	curr->do0 &= ~0xc0;
	if (curr->v21 < 0) {
		usec = curr->v21 * -1000;
		curr->do0 |=  0x80;
	} else {
		usec = curr->v21 *  1000;
		curr->do0 |=  0x40;
	}
	if (usec > 5000000)
		usec = 5000000;

	set_parallel_output_status(1, curr->do0);

	usleep(usec);

	curr->do0 &= ~0xc0;
	set_parallel_output_status(1, curr->do0);

	return usec;
}

void
log_status(struct site_status *site_status)
{
	syslog(LOG_INFO, "T %3i, T11 %3i, T12 %3i, T21 %3i, P11 %3u, P12 %3u, "
			"V1 %3i, V2 %3i\n", site_status->t,
		       	site_status->t11, site_status->t12, site_status->t21,
		       	site_status->p11, site_status->p12, site_status->v11,
		       	site_status->v21);
	debug("T %3i, T11 %2i, T12 %2i, T21 %2i, P11 %3u, P12 %3u, "
			"V11 %2i, V21 %2i\n", site_status->t,
		       	site_status->t11, site_status->t12, site_status->t21,
		       	site_status->p11, site_status->p12, site_status->v11,
		       	site_status->v21);
	debug("error: T11 %3i, T12 %3i, T21 %3i"
			"\n",
		       	site_status->e11, site_status->e12,
		       	site_status->e21);
}

int
main(int argc, char **argv)
{
	unsigned c = 0;
	unsigned t;
	struct site_status s1 = {0}, s2 = {0};
	struct site_status *curr, *prev;

	openlog("pcs", 0, LOG_DAEMON);
	load_site_status(&s2);
	calculate_e1(&s2);
	calculate_e2(&s2);
	while (1) {
		c = c ^ 1;
		if (c) {
			curr = &s1;
			prev = &s2;
		} else {
			curr = &s2;
			prev = &s1;
		}
		load_site_status(curr);
		calculate_e1(curr);
		calculate_v11(curr, prev);
		calculate_e2(curr);
		calculate_v21(curr, prev);
		log_status(curr);
		t = 10000000;
		t -= execute_v11(curr);;
		t -= execute_v21(curr);;
		usleep(t);
	}
	return 0;
}
