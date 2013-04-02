#include <unistd.h>
#include <stdio.h>
#include <time.h>

void
run(void)
{
	char buff[24];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	strftime(&buff[0], sizeof(buff) - 1, "%b %e %H:%M:%S", &tm);
	printf("%s Greetings, world!\n", buff);
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
